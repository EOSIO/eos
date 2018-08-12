#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>

namespace eosio { namespace chain {

   transaction_context::transaction_context( controller& c,
                                             const signed_transaction& t,
                                             const transaction_id_type& trx_id,
                                             fc::time_point s )
   :control(c)
   ,trx(t)
   ,id(trx_id)
   ,undo_session(c.db().start_undo_session(true))
   ,trace(std::make_shared<transaction_trace>())
   ,start(s)
   ,net_usage(trace->net_usage)
   ,pseudo_start(s)
   {
      trace->id = id;
      executed.reserve( trx.total_actions() );
      EOS_ASSERT( trx.transaction_extensions.size() == 0, unsupported_feature, "we don't support any extensions yet" );
   }

   void transaction_context::init(uint64_t initial_net_usage)
   {
      EOS_ASSERT( !is_initialized, transaction_exception, "cannot initialize twice" );
      const static int64_t large_number_no_overflow = std::numeric_limits<int64_t>::max()/2;

      const auto& cfg = control.get_global_properties().configuration;
      auto& rl = control.get_mutable_resource_limits_manager();

      net_limit = rl.get_block_net_limit();

      objective_duration_limit = fc::microseconds( rl.get_block_cpu_limit() );
      _deadline = start + objective_duration_limit;

      // Possibly lower net_limit to the maximum net usage a transaction is allowed to be billed
      if( cfg.max_transaction_net_usage <= net_limit ) {
         net_limit = cfg.max_transaction_net_usage;
         net_limit_due_to_block = false;
      }

      // Possibly lower objective_duration_limit to the maximum cpu usage a transaction is allowed to be billed
      if( cfg.max_transaction_cpu_usage <= objective_duration_limit.count() ) {
         objective_duration_limit = fc::microseconds(cfg.max_transaction_cpu_usage);
         billing_timer_exception_code = tx_cpu_usage_exceeded::code_value;
         _deadline = start + objective_duration_limit;
      }

      // Possibly lower net_limit to optional limit set in the transaction header
      uint64_t trx_specified_net_usage_limit = static_cast<uint64_t>(trx.max_net_usage_words.value) * 8;
      if( trx_specified_net_usage_limit > 0 && trx_specified_net_usage_limit <= net_limit ) {
         net_limit = trx_specified_net_usage_limit;
         net_limit_due_to_block = false;
      }

      // Possibly lower objective_duration_limit to optional limit set in transaction header
      if( trx.max_cpu_usage_ms > 0 ) {
         auto trx_specified_cpu_usage_limit = fc::milliseconds(trx.max_cpu_usage_ms);
         if( trx_specified_cpu_usage_limit <= objective_duration_limit ) {
            objective_duration_limit = trx_specified_cpu_usage_limit;
            billing_timer_exception_code = tx_cpu_usage_exceeded::code_value;
            _deadline = start + objective_duration_limit;
         }
      }

      initial_objective_duration_limit = objective_duration_limit;

      if( billed_cpu_time_us > 0 ) // could also call on explicit_billed_cpu_time but it would be redundant
         validate_cpu_usage_to_bill( billed_cpu_time_us, false ); // Fail early if the amount to be billed is too high

      // Record accounts to be billed for network and CPU usage
      for( const auto& act : trx.actions ) {
         for( const auto& auth : act.authorization ) {
            bill_to_accounts.insert( auth.actor );
         }
      }
      validate_ram_usage.reserve( bill_to_accounts.size() );

      // Update usage values of accounts to reflect new time
      rl.update_account_usage( bill_to_accounts, block_timestamp_type(control.pending_block_time()).slot );

      // Calculate the highest network usage and CPU time that all of the billed accounts can afford to be billed
      int64_t account_net_limit = 0;
      int64_t account_cpu_limit = 0;
      bool greylisted = false;
      std::tie( account_net_limit, account_cpu_limit, greylisted ) = max_bandwidth_billed_accounts_can_pay();
      net_limit_due_to_greylist |= greylisted;

      eager_net_limit = net_limit;

      // Possible lower eager_net_limit to what the billed accounts can pay plus some (objective) leeway
      auto new_eager_net_limit = std::min( eager_net_limit, static_cast<uint64_t>(account_net_limit + cfg.net_usage_leeway) );
      if( new_eager_net_limit < eager_net_limit ) {
         eager_net_limit = new_eager_net_limit;
         net_limit_due_to_block = false;
      }

      // Possibly limit deadline if the duration accounts can be billed for (+ a subjective leeway) does not exceed current delta
      if( (fc::microseconds(account_cpu_limit) + leeway) <= (_deadline - start) ) {
         _deadline = start + fc::microseconds(account_cpu_limit) + leeway;
         billing_timer_exception_code = leeway_deadline_exception::code_value;
      }

      billing_timer_duration_limit = _deadline - start;

      // Check if deadline is limited by caller-set deadline (only change deadline if billed_cpu_time_us is not set)
      if( explicit_billed_cpu_time || deadline < _deadline ) {
         _deadline = deadline;
         deadline_exception_code = deadline_exception::code_value;
      } else {
         deadline_exception_code = billing_timer_exception_code;
      }

      eager_net_limit = (eager_net_limit/8)*8; // Round down to nearest multiple of word size (8 bytes) so check_net_usage can be efficient

      if( initial_net_usage > 0 )
         add_net_usage( initial_net_usage );  // Fail early if current net usage is already greater than the calculated limit

      checktime(); // Fail early if deadline has already been exceeded

      is_initialized = true;
   }

   void transaction_context::init_for_implicit_trx( uint64_t initial_net_usage  )
   {
      published = control.pending_block_time();
      init( initial_net_usage);
   }

   void transaction_context::init_for_input_trx( uint64_t packed_trx_unprunable_size,
                                                 uint64_t packed_trx_prunable_size,
                                                 uint32_t num_signatures)
   {
      const auto& cfg = control.get_global_properties().configuration;

      uint64_t discounted_size_for_pruned_data = packed_trx_prunable_size;
      if( cfg.context_free_discount_net_usage_den > 0
          && cfg.context_free_discount_net_usage_num < cfg.context_free_discount_net_usage_den )
      {
         discounted_size_for_pruned_data *= cfg.context_free_discount_net_usage_num;
         discounted_size_for_pruned_data =  ( discounted_size_for_pruned_data + cfg.context_free_discount_net_usage_den - 1)
                                                                                    / cfg.context_free_discount_net_usage_den; // rounds up
      }

      uint64_t initial_net_usage = static_cast<uint64_t>(cfg.base_per_transaction_net_usage)
                                    + packed_trx_unprunable_size + discounted_size_for_pruned_data;


      if( trx.delay_sec.value > 0 ) {
          // If delayed, also charge ahead of time for the additional net usage needed to retire the delayed transaction
          // whether that be by successfully executing, soft failure, hard failure, or expiration.
         initial_net_usage += static_cast<uint64_t>(cfg.base_per_transaction_net_usage)
                               + static_cast<uint64_t>(config::transaction_id_net_usage);
      }

      published = control.pending_block_time();
      is_input = true;
      control.validate_expiration( trx );
      control.validate_tapos( trx );
      control.validate_referenced_accounts( trx );
      init( initial_net_usage);
      record_transaction( id, trx.expiration ); /// checks for dupes
   }

   void transaction_context::init_for_deferred_trx( fc::time_point p )
   {
      published = p;
      trace->scheduled = true;
      apply_context_free = false;
      init( 0 );
   }

   void transaction_context::exec() {
      EOS_ASSERT( is_initialized, transaction_exception, "must first initialize" );

      if( apply_context_free ) {
         for( const auto& act : trx.context_free_actions ) {
            trace->action_traces.emplace_back();
            dispatch_action( trace->action_traces.back(), act, true );
         }
      }

      if( delay == fc::microseconds() ) {
         for( const auto& act : trx.actions ) {
            trace->action_traces.emplace_back();
            dispatch_action( trace->action_traces.back(), act );
         }
      } else {
         schedule_transaction();
      }
   }

   void transaction_context::finalize() {
      EOS_ASSERT( is_initialized, transaction_exception, "must first initialize" );

      if( is_input ) {
         auto& am = control.get_mutable_authorization_manager();
         for( const auto& act : trx.actions ) {
            for( const auto& auth : act.authorization ) {
               am.update_permission_usage( am.get_permission(auth) );
            }
         }
      }

      auto& rl = control.get_mutable_resource_limits_manager();
      for( auto a : validate_ram_usage ) {
         rl.verify_account_ram_usage( a );
      }

      // Calculate the new highest network usage and CPU time that all of the billed accounts can afford to be billed
      int64_t account_net_limit = 0;
      int64_t account_cpu_limit = 0;
      bool greylisted = false;
      std::tie( account_net_limit, account_cpu_limit, greylisted ) = max_bandwidth_billed_accounts_can_pay();
      net_limit_due_to_greylist |= greylisted;

      // Possibly lower net_limit to what the billed accounts can pay
      if( static_cast<uint64_t>(account_net_limit) <= net_limit ) {
         net_limit = static_cast<uint64_t>(account_net_limit);
         net_limit_due_to_block = false;
      }

      // Possibly lower objective_duration_limit to what the billed accounts can pay
      if( account_cpu_limit <= objective_duration_limit.count() ) {
         objective_duration_limit = fc::microseconds(account_cpu_limit);
         billing_timer_exception_code = tx_cpu_usage_exceeded::code_value;
      }

      net_usage = ((net_usage + 7)/8)*8; // Round up to nearest multiple of word size (8 bytes)

      eager_net_limit = net_limit;
      check_net_usage();

      auto now = fc::time_point::now();
      trace->elapsed = now - start;

      update_billed_cpu_time( now );

      validate_cpu_usage_to_bill( billed_cpu_time_us );

      rl.add_transaction_usage( bill_to_accounts, static_cast<uint64_t>(billed_cpu_time_us), net_usage,
                                block_timestamp_type(control.pending_block_time()).slot ); // Should never fail
   }

   void transaction_context::squash() {
      undo_session.squash();
   }

   void transaction_context::undo() {
      undo_session.undo();
   }

   void transaction_context::check_net_usage()const {
      if( BOOST_UNLIKELY(net_usage > eager_net_limit) ) {
         if ( net_limit_due_to_block ) {
            EOS_THROW( block_net_usage_exceeded,
                       "not enough space left in block: ${net_usage} > ${net_limit}",
                       ("net_usage", net_usage)("net_limit", eager_net_limit) );
         }  else if (net_limit_due_to_greylist) {
            EOS_THROW( greylist_net_usage_exceeded,
                       "net usage of transaction is too high: ${net_usage} > ${net_limit}",
                       ("net_usage", net_usage)("net_limit", eager_net_limit) );
         } else {
            EOS_THROW( tx_net_usage_exceeded,
                       "net usage of transaction is too high: ${net_usage} > ${net_limit}",
                       ("net_usage", net_usage)("net_limit", eager_net_limit) );
         }
      }
   }

   void transaction_context::checktime()const {
      auto now = fc::time_point::now();
      if( BOOST_UNLIKELY( now > _deadline ) ) {
         // edump((now-start)(now-pseudo_start));
         if( explicit_billed_cpu_time || deadline_exception_code == deadline_exception::code_value ) {
            EOS_THROW( deadline_exception, "deadline exceeded", ("now", now)("deadline", _deadline)("start", start) );
         } else if( deadline_exception_code == block_cpu_usage_exceeded::code_value ) {
            EOS_THROW( block_cpu_usage_exceeded,
                       "not enough time left in block to complete executing transaction",
                       ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - pseudo_start) );
         } else if( deadline_exception_code == tx_cpu_usage_exceeded::code_value ) {
            EOS_THROW( tx_cpu_usage_exceeded,
                       "transaction was executing for too long",
                       ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - pseudo_start) );
         } else if( deadline_exception_code == leeway_deadline_exception::code_value ) {
            EOS_THROW( leeway_deadline_exception,
                       "the transaction was unable to complete by deadline, "
                       "but it is possible it could have succeeded if it were allowed to run to completion",
                       ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - pseudo_start) );
         }
         EOS_ASSERT( false,  transaction_exception, "unexpected deadline exception code" );
      }
   }

   void transaction_context::pause_billing_timer() {
      if( explicit_billed_cpu_time || pseudo_start == fc::time_point() ) return; // either irrelevant or already paused

      auto now = fc::time_point::now();
      billed_time = now - pseudo_start;
      deadline_exception_code = deadline_exception::code_value; // Other timeout exceptions cannot be thrown while billable timer is paused.
      pseudo_start = fc::time_point();
   }

   void transaction_context::resume_billing_timer() {
      if( explicit_billed_cpu_time || pseudo_start != fc::time_point() ) return; // either irrelevant or already running

      auto now = fc::time_point::now();
      pseudo_start = now - billed_time;
      if( (pseudo_start + billing_timer_duration_limit) <= deadline ) {
         _deadline = pseudo_start + billing_timer_duration_limit;
         deadline_exception_code = billing_timer_exception_code;
      } else {
         _deadline = deadline;
         deadline_exception_code = deadline_exception::code_value;
      }
   }

   void transaction_context::validate_cpu_usage_to_bill( int64_t billed_us, bool check_minimum )const {
      if( check_minimum ) {
         const auto& cfg = control.get_global_properties().configuration;
         EOS_ASSERT( billed_us >= cfg.min_transaction_cpu_usage, transaction_exception,
                     "cannot bill CPU time less than the minimum of ${min_billable} us",
                     ("min_billable", cfg.min_transaction_cpu_usage)("billed_cpu_time_us", billed_us)
                   );
      }

      if( billing_timer_exception_code == block_cpu_usage_exceeded::code_value ) {
         EOS_ASSERT( billed_us <= objective_duration_limit.count(),
                     block_cpu_usage_exceeded,
                     "billed CPU time (${billed} us) is greater than the billable CPU time left in the block (${billable} us)",
                     ("billed", billed_us)("billable", objective_duration_limit.count())
                   );
      } else {
         EOS_ASSERT( billed_us <= objective_duration_limit.count(),
                     tx_cpu_usage_exceeded,
                     "billed CPU time (${billed} us) is greater than the maximum billable CPU time for the transaction (${billable} us)",
                     ("billed", billed_us)("billable", objective_duration_limit.count())
                   );
      }
   }

   void transaction_context::add_ram_usage( account_name account, int64_t ram_delta ) {
      auto& rl = control.get_mutable_resource_limits_manager();
      rl.add_pending_ram_usage( account, ram_delta );
      if( ram_delta > 0 ) {
         validate_ram_usage.insert( account );
      }
   }

   uint32_t transaction_context::update_billed_cpu_time( fc::time_point now ) {
      if( explicit_billed_cpu_time ) return static_cast<uint32_t>(billed_cpu_time_us);

      const auto& cfg = control.get_global_properties().configuration;
      billed_cpu_time_us = std::max( (now - pseudo_start).count(), static_cast<int64_t>(cfg.min_transaction_cpu_usage) );

      return static_cast<uint32_t>(billed_cpu_time_us);
   }

   std::tuple<int64_t, int64_t, bool> transaction_context::max_bandwidth_billed_accounts_can_pay( bool force_elastic_limits )const {
      // Assumes rl.update_account_usage( bill_to_accounts, block_timestamp_type(control.pending_block_time()).slot ) was already called prior

      // Calculate the new highest network usage and CPU time that all of the billed accounts can afford to be billed
      auto& rl = control.get_mutable_resource_limits_manager();
      const static int64_t large_number_no_overflow = std::numeric_limits<int64_t>::max()/2;
      int64_t account_net_limit = large_number_no_overflow;
      int64_t account_cpu_limit = large_number_no_overflow;
      bool greylisted = false;
      for( const auto& a : bill_to_accounts ) {
         bool elastic = force_elastic_limits || !(control.is_producing_block() && control.is_resource_greylisted(a));
         auto net_limit = rl.get_account_net_limit(a, elastic);
         if( net_limit >= 0 ) {
            account_net_limit = std::min( account_net_limit, net_limit );
            if (!elastic) greylisted = true;
         }
         auto cpu_limit = rl.get_account_cpu_limit(a, elastic);
         if( cpu_limit >= 0 )
            account_cpu_limit = std::min( account_cpu_limit, cpu_limit );
      }

      return std::make_tuple(account_net_limit, account_cpu_limit, greylisted);
   }

   void transaction_context::dispatch_action( action_trace& trace, const action& a, account_name receiver, bool context_free, uint32_t recurse_depth ) {
      apply_context  acontext( control, *this, a, recurse_depth );
      acontext.context_free = context_free;
      acontext.receiver     = receiver;

      try {
         acontext.exec();
      } catch( ... ) {
         trace = move(acontext.trace);
         throw;
      }

      trace = move(acontext.trace);
   }

   void transaction_context::schedule_transaction() {
      // Charge ahead of time for the additional net usage needed to retire the delayed transaction
      // whether that be by successfully executing, soft failure, hard failure, or expiration.
      if( trx.delay_sec.value == 0 ) { // Do not double bill. Only charge if we have not already charged for the delay.
         const auto& cfg = control.get_global_properties().configuration;
         add_net_usage( static_cast<uint64_t>(cfg.base_per_transaction_net_usage)
                         + static_cast<uint64_t>(config::transaction_id_net_usage) ); // Will exit early if net usage cannot be payed.
      }

      auto first_auth = trx.first_authorizor();

      uint32_t trx_size = 0;
      const auto& cgto = control.db().create<generated_transaction_object>( [&]( auto& gto ) {
        gto.trx_id      = id;
        gto.payer       = first_auth;
        gto.sender      = account_name(); /// delayed transactions have no sender
        gto.sender_id   = transaction_id_to_sender_id( gto.trx_id );
        gto.published   = control.pending_block_time();
        gto.delay_until = gto.published + delay;
        gto.expiration  = gto.delay_until + fc::seconds(control.get_global_properties().configuration.deferred_trx_expiration_window);
        trx_size = gto.set( trx );
      });

      add_ram_usage( cgto.payer, (config::billable_size_v<generated_transaction_object> + trx_size) );
   }

   void transaction_context::record_transaction( const transaction_id_type& id, fc::time_point_sec expire ) {
      try {
          control.db().create<transaction_object>([&](transaction_object& transaction) {
              transaction.trx_id = id;
              transaction.expiration = expire;
          });
      } catch( const boost::interprocess::bad_alloc& ) {
         throw;
      } catch ( ... ) {
          EOS_ASSERT( false, tx_duplicate,
                     "duplicate transaction ${id}", ("id", id ) );
      }
   } /// record_transaction


} } /// eosio::chain
