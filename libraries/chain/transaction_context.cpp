#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>

#pragma push_macro("N")
#undef N
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
#include <boost/accumulators/statistics/weighted_variance.hpp>
#pragma pop_macro("N")

#include <chrono>

namespace eosio { namespace chain {

   transaction_checktime_timer::transaction_checktime_timer(platform_timer& timer)
         : expired(timer.expired), _timer(timer) {
      expired = 0;
   }

   void transaction_checktime_timer::start(fc::time_point tp) {
      _timer.start(tp);
   }

   void transaction_checktime_timer::stop() {
      _timer.stop();
   }

   void transaction_checktime_timer::set_expiration_callback(void(*func)(void*), void* user) {
      _timer.set_expiration_callback(func, user);
   }

   transaction_checktime_timer::~transaction_checktime_timer() {
      stop();
      _timer.set_expiration_callback(nullptr, nullptr);
   }

   transaction_context::transaction_context( controller& c,
                                             const signed_transaction& t,
                                             const transaction_id_type& trx_id,
                                             transaction_checktime_timer&& tmr,
                                             fc::time_point s )
   :control(c)
   ,trx(t)
   ,id(trx_id)
   ,undo_session()
   ,trace(std::make_shared<transaction_trace>())
   ,start(s)
   ,transaction_timer(std::move(tmr))
   ,net_usage(trace->net_usage)
   ,pseudo_start(s)
   {
      if (!c.skip_db_sessions()) {
         undo_session = c.mutable_db().start_undo_session(true);
      }
      trace->id = id;
      trace->block_num = c.head_block_num() + 1;
      trace->block_time = c.pending_block_time();
      trace->producer_block_id = c.pending_producer_block_id();
      executed.reserve( trx.total_actions() );
   }

   void transaction_context::disallow_transaction_extensions( const char* error_msg )const {
      if( control.is_producing_block() ) {
         EOS_THROW( subjective_block_production_exception, error_msg );
      } else {
         EOS_THROW( disallowed_transaction_extensions_bad_block_exception, error_msg );
      }
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

      if( explicit_billed_cpu_time )
         validate_cpu_usage_to_bill( billed_cpu_time_us, std::numeric_limits<int64_t>::max(), false ); // Fail early if the amount to be billed is too high

      // Record accounts to be billed for network and CPU usage
      if( control.is_builtin_activated(builtin_protocol_feature_t::only_bill_first_authorizer) ) {
         bill_to_accounts.insert( trx.first_authorizer() );
      } else {
         for( const auto& act : trx.actions ) {
            for( const auto& auth : act.authorization ) {
               bill_to_accounts.insert( auth.actor );
            }
         }
      }
      validate_ram_usage.reserve( bill_to_accounts.size() );

      // Update usage values of accounts to reflect new time
      rl.update_account_usage( bill_to_accounts, block_timestamp_type(control.pending_block_time()).slot );

      // Calculate the highest network usage and CPU time that all of the billed accounts can afford to be billed
      int64_t account_net_limit = 0;
      int64_t account_cpu_limit = 0;
      bool greylisted_net = false, greylisted_cpu = false;
      std::tie( account_net_limit, account_cpu_limit, greylisted_net, greylisted_cpu) = max_bandwidth_billed_accounts_can_pay();
      net_limit_due_to_greylist |= greylisted_net;
      cpu_limit_due_to_greylist |= greylisted_cpu;

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

      if( !explicit_billed_cpu_time ) {
         // Fail early if amount of the previous speculative execution is within 10% of remaining account cpu available
         int64_t validate_account_cpu_limit = account_cpu_limit - subjective_cpu_bill_us;
         if( validate_account_cpu_limit > 0 )
            validate_account_cpu_limit -= EOS_PERCENT( validate_account_cpu_limit, 10 * config::percent_1 );
         if( validate_account_cpu_limit < 0 ) validate_account_cpu_limit = 0;
         validate_account_cpu_usage( billed_cpu_time_us, validate_account_cpu_limit, true );
      }

      eager_net_limit = (eager_net_limit/8)*8; // Round down to nearest multiple of word size (8 bytes) so check_net_usage can be efficient

      if( initial_net_usage > 0 )
         add_net_usage( initial_net_usage );  // Fail early if current net usage is already greater than the calculated limit

      checktime(); // Fail early if deadline has already been exceeded

      if(control.skip_trx_checks())
         transaction_timer.start(fc::time_point::maximum());
      else
         transaction_timer.start(_deadline);

      is_initialized = true;
   }

   void transaction_context::init_for_implicit_trx( uint64_t initial_net_usage  )
   {
      if( trx.transaction_extensions.size() > 0 ) {
         disallow_transaction_extensions( "no transaction extensions supported yet for implicit transactions" );
      }

      published = control.pending_block_time();
      init( initial_net_usage);
   }

   void transaction_context::init_for_input_trx( uint64_t packed_trx_unprunable_size,
                                                 uint64_t packed_trx_prunable_size,
                                                 bool skip_recording )
   {
      if( trx.transaction_extensions.size() > 0 ) {
         disallow_transaction_extensions( "no transaction extensions supported yet for input transactions" );
      }

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
      if (!control.skip_trx_checks()) {
         control.validate_expiration(trx);
         control.validate_tapos(trx);
         validate_referenced_accounts( trx, enforce_whiteblacklist && control.is_producing_block() );
      }
      init( initial_net_usage);
      if (!skip_recording)
         record_transaction( id, trx.expiration ); /// checks for dupes
   }

   void transaction_context::init_for_deferred_trx( fc::time_point p )
   {
      if( (trx.expiration.sec_since_epoch() != 0) && (trx.transaction_extensions.size() > 0) ) {
         disallow_transaction_extensions( "no transaction extensions supported yet for deferred transactions" );
      }
      // If (trx.expiration.sec_since_epoch() == 0) then it was created after NO_DUPLICATE_DEFERRED_ID activation,
      // and so validation of its extensions was done either in:
      //   * apply_context::schedule_deferred_transaction for contract-generated transactions;
      //   * or transaction_context::init_for_input_trx for delayed input transactions.

      published = p;
      trace->scheduled = true;
      apply_context_free = false;
      init( 0 );
   }

   void transaction_context::exec() {
      EOS_ASSERT( is_initialized, transaction_exception, "must first initialize" );

      if( apply_context_free ) {
         for( const auto& act : trx.context_free_actions ) {
            schedule_action( act, act.account, true, 0, 0 );
         }
      }

      if( delay == fc::microseconds() ) {
         for( const auto& act : trx.actions ) {
            schedule_action( act, act.account, false, 0, 0 );
         }
      }

      auto& action_traces = trace->action_traces;
      uint32_t num_original_actions_to_execute = action_traces.size();
      for( uint32_t i = 1; i <= num_original_actions_to_execute; ++i ) {
         execute_action( i, 0 );
      }

      if( delay != fc::microseconds() ) {
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
      bool greylisted_net = false, greylisted_cpu = false;
      std::tie( account_net_limit, account_cpu_limit, greylisted_net, greylisted_cpu) = max_bandwidth_billed_accounts_can_pay();
      net_limit_due_to_greylist |= greylisted_net;
      cpu_limit_due_to_greylist |= greylisted_cpu;

      // Possibly lower net_limit to what the billed accounts can pay
      if( static_cast<uint64_t>(account_net_limit) <= net_limit ) {
         // NOTE: net_limit may possibly not be objective anymore due to net greylisting, but it should still be no greater than the truly objective net_limit
         net_limit = static_cast<uint64_t>(account_net_limit);
         net_limit_due_to_block = false;
      }

      // Possibly lower objective_duration_limit to what the billed accounts can pay
      if( account_cpu_limit <= objective_duration_limit.count() ) {
         // NOTE: objective_duration_limit may possibly not be objective anymore due to cpu greylisting, but it should still be no greater than the truly objective objective_duration_limit
         objective_duration_limit = fc::microseconds(account_cpu_limit);
         billing_timer_exception_code = tx_cpu_usage_exceeded::code_value;
      }

      net_usage = ((net_usage + 7)/8)*8; // Round up to nearest multiple of word size (8 bytes)

      eager_net_limit = net_limit;
      check_net_usage();

      auto now = fc::time_point::now();
      trace->elapsed = now - start;

      update_billed_cpu_time( now );

      validate_cpu_usage_to_bill( billed_cpu_time_us, account_cpu_limit, true );

      rl.add_transaction_usage( bill_to_accounts, static_cast<uint64_t>(billed_cpu_time_us), net_usage,
                                block_timestamp_type(control.pending_block_time()).slot ); // Should never fail
   }

   void transaction_context::squash() {
      if (undo_session) undo_session->squash();
   }

   void transaction_context::undo() {
      if (undo_session) undo_session->undo();
   }

   void transaction_context::check_net_usage()const {
      if (!control.skip_trx_checks()) {
         if( BOOST_UNLIKELY(net_usage > eager_net_limit) ) {
            if ( net_limit_due_to_block ) {
               EOS_THROW( block_net_usage_exceeded,
                          "not enough space left in block: ${net_usage} > ${net_limit}",
                          ("net_usage", net_usage)("net_limit", eager_net_limit) );
            }  else if (net_limit_due_to_greylist) {
               EOS_THROW( greylist_net_usage_exceeded,
                          "greylisted transaction net usage is too high: ${net_usage} > ${net_limit}",
                          ("net_usage", net_usage)("net_limit", eager_net_limit) );
            } else {
               EOS_THROW( tx_net_usage_exceeded,
                          "transaction net usage is too high: ${net_usage} > ${net_limit}",
                          ("net_usage", net_usage)("net_limit", eager_net_limit) );
            }
         }
      }
   }

   void transaction_context::checktime()const {
      if(BOOST_LIKELY(transaction_timer.expired == false))
         return;

      auto now = fc::time_point::now();
      if( explicit_billed_cpu_time || deadline_exception_code == deadline_exception::code_value ) {
         EOS_THROW( deadline_exception, "deadline exceeded ${billing_timer}us",
                     ("billing_timer", now - pseudo_start)("now", now)("deadline", _deadline)("start", start) );
      } else if( deadline_exception_code == block_cpu_usage_exceeded::code_value ) {
         EOS_THROW( block_cpu_usage_exceeded,
                     "not enough time left in block to complete executing transaction ${billing_timer}us",
                     ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - pseudo_start) );
      } else if( deadline_exception_code == tx_cpu_usage_exceeded::code_value ) {
         if (cpu_limit_due_to_greylist) {
            EOS_THROW( greylist_cpu_usage_exceeded,
                     "greylisted transaction was executing for too long ${billing_timer}us",
                     ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - pseudo_start) );
         } else {
            EOS_THROW( tx_cpu_usage_exceeded,
                     "transaction was executing for too long ${billing_timer}us",
                     ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - pseudo_start) );
         }
      } else if( deadline_exception_code == leeway_deadline_exception::code_value ) {
         EOS_THROW( leeway_deadline_exception,
                     "the transaction was unable to complete by deadline, "
                     "but it is possible it could have succeeded if it were allowed to run to completion ${billing_timer}",
                     ("now", now)("deadline", _deadline)("start", start)("billing_timer", now - pseudo_start) );
      }
      EOS_ASSERT( false,  transaction_exception, "unexpected deadline exception code ${code}", ("code", deadline_exception_code) );
   }

   void transaction_context::pause_billing_timer() {
      if( explicit_billed_cpu_time || pseudo_start == fc::time_point() ) return; // either irrelevant or already paused

      auto now = fc::time_point::now();
      billed_time = now - pseudo_start;
      deadline_exception_code = deadline_exception::code_value; // Other timeout exceptions cannot be thrown while billable timer is paused.
      pseudo_start = fc::time_point();
      transaction_timer.stop();
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
      transaction_timer.start(_deadline);
   }

   void transaction_context::validate_cpu_usage_to_bill( int64_t billed_us, int64_t account_cpu_limit, bool check_minimum )const {
      if (!control.skip_trx_checks()) {
         if( check_minimum ) {
            const auto& cfg = control.get_global_properties().configuration;
            EOS_ASSERT( billed_us >= cfg.min_transaction_cpu_usage, transaction_exception,
                        "cannot bill CPU time less than the minimum of ${min_billable} us",
                        ("min_billable", cfg.min_transaction_cpu_usage)("billed_cpu_time_us", billed_us)
                      );
         }

         validate_account_cpu_usage( billed_us, account_cpu_limit, false );
      }
   }

   void transaction_context::validate_account_cpu_usage( int64_t billed_us, int64_t account_cpu_limit, bool estimate )const {
      if( (billed_us > 0) && !control.skip_trx_checks() ) {
         const bool cpu_limited_by_account = (account_cpu_limit <= objective_duration_limit.count());

         if( !cpu_limited_by_account && (billing_timer_exception_code == block_cpu_usage_exceeded::code_value) ) {
            EOS_ASSERT( billed_us <= objective_duration_limit.count(),
                        block_cpu_usage_exceeded,
                        "${desc} CPU time (${billed} us) is greater than the billable CPU time left in the block (${billable} us)",
                        ("desc", (estimate ? "estimated" : "billed"))("billed", billed_us)( "billable", objective_duration_limit.count() )
            );
         } else {
            if( cpu_limit_due_to_greylist && cpu_limited_by_account ) {
               EOS_ASSERT( billed_us <= account_cpu_limit,
                           greylist_cpu_usage_exceeded,
                           "${desc} CPU time (${billed} us) is greater than the maximum greylisted billable CPU time for the transaction (${billable} us)",
                           ("desc", (estimate ? "estimated" : "billed"))("billed", billed_us)( "billable", account_cpu_limit )
               );
            } else {
               // exceeds trx.max_cpu_usage_ms or cfg.max_transaction_cpu_usage if objective_duration_limit is greater
               const int64_t cpu_limit = (cpu_limited_by_account ? account_cpu_limit : objective_duration_limit.count());
               EOS_ASSERT( billed_us <= cpu_limit,
                           tx_cpu_usage_exceeded,
                           "${desc} CPU time (${billed} us) is greater than the maximum billable CPU time for the transaction (${billable} us)",
                           ("desc", (estimate ? "estimated" : "billed"))("billed", billed_us)( "billable", cpu_limit )
               );
            }
         }
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

   std::tuple<int64_t, int64_t, bool, bool> transaction_context::max_bandwidth_billed_accounts_can_pay( bool force_elastic_limits ) const{
      // Assumes rl.update_account_usage( bill_to_accounts, block_timestamp_type(control.pending_block_time()).slot ) was already called prior

      // Calculate the new highest network usage and CPU time that all of the billed accounts can afford to be billed
      auto& rl = control.get_mutable_resource_limits_manager();
      const static int64_t large_number_no_overflow = std::numeric_limits<int64_t>::max()/2;
      int64_t account_net_limit = large_number_no_overflow;
      int64_t account_cpu_limit = large_number_no_overflow;
      bool greylisted_net = false;
      bool greylisted_cpu = false;

      uint32_t specified_greylist_limit = control.get_greylist_limit();
      for( const auto& a : bill_to_accounts ) {
         uint32_t greylist_limit = config::maximum_elastic_resource_multiplier;
         if( !force_elastic_limits && control.is_producing_block() ) {
            if( control.is_resource_greylisted(a) ) {
               greylist_limit = 1;
            } else {
               greylist_limit = specified_greylist_limit;
            }
         }
         auto [net_limit, net_was_greylisted] = rl.get_account_net_limit(a, greylist_limit);
         if( net_limit >= 0 ) {
            account_net_limit = std::min( account_net_limit, net_limit );
            greylisted_net |= net_was_greylisted;
         }
         auto [cpu_limit, cpu_was_greylisted] = rl.get_account_cpu_limit(a, greylist_limit);
         if( cpu_limit >= 0 ) {
            account_cpu_limit = std::min( account_cpu_limit, cpu_limit );
            greylisted_cpu |= cpu_was_greylisted;
         }
      }

      EOS_ASSERT( (!force_elastic_limits && control.is_producing_block()) || (!greylisted_cpu && !greylisted_net),
                  transaction_exception, "greylisted when not producing block" );

      return std::make_tuple(account_net_limit, account_cpu_limit, greylisted_net, greylisted_cpu);
   }

   action_trace& transaction_context::get_action_trace( uint32_t action_ordinal ) {
      EOS_ASSERT( 0 < action_ordinal && action_ordinal <= trace->action_traces.size() ,
                  transaction_exception,
                  "action_ordinal ${ordinal} is outside allowed range [1,${max}]",
                  ("ordinal", action_ordinal)("max", trace->action_traces.size())
      );
      return trace->action_traces[action_ordinal-1];
   }

   const action_trace& transaction_context::get_action_trace( uint32_t action_ordinal )const {
      EOS_ASSERT( 0 < action_ordinal && action_ordinal <= trace->action_traces.size() ,
                  transaction_exception,
                  "action_ordinal ${ordinal} is outside allowed range [1,${max}]",
                  ("ordinal", action_ordinal)("max", trace->action_traces.size())
      );
      return trace->action_traces[action_ordinal-1];
   }

   uint32_t transaction_context::schedule_action( const action& act, account_name receiver, bool context_free,
                                                  uint32_t creator_action_ordinal,
                                                  uint32_t closest_unnotified_ancestor_action_ordinal )
   {
      uint32_t new_action_ordinal = trace->action_traces.size() + 1;

      trace->action_traces.emplace_back( *trace, act, receiver, context_free,
                                         new_action_ordinal, creator_action_ordinal,
                                         closest_unnotified_ancestor_action_ordinal );

      return new_action_ordinal;
   }

   uint32_t transaction_context::schedule_action( action&& act, account_name receiver, bool context_free,
                                                  uint32_t creator_action_ordinal,
                                                  uint32_t closest_unnotified_ancestor_action_ordinal )
   {
      uint32_t new_action_ordinal = trace->action_traces.size() + 1;

      trace->action_traces.emplace_back( *trace, std::move(act), receiver, context_free,
                                         new_action_ordinal, creator_action_ordinal,
                                         closest_unnotified_ancestor_action_ordinal );

      return new_action_ordinal;
   }

   uint32_t transaction_context::schedule_action( uint32_t action_ordinal, account_name receiver, bool context_free,
                                                  uint32_t creator_action_ordinal,
                                                  uint32_t closest_unnotified_ancestor_action_ordinal )
   {
      uint32_t new_action_ordinal = trace->action_traces.size() + 1;

      trace->action_traces.reserve( new_action_ordinal );

      const action& provided_action = get_action_trace( action_ordinal ).act;

      // The reserve above is required so that the emplace_back below does not invalidate the provided_action reference.

      trace->action_traces.emplace_back( *trace, provided_action, receiver, context_free,
                                         new_action_ordinal, creator_action_ordinal,
                                         closest_unnotified_ancestor_action_ordinal );

      return new_action_ordinal;
   }

   void transaction_context::execute_action( uint32_t action_ordinal, uint32_t recurse_depth ) {
      apply_context acontext( control, *this, action_ordinal, recurse_depth );
      acontext.exec();
   }


   void transaction_context::schedule_transaction() {
      // Charge ahead of time for the additional net usage needed to retire the delayed transaction
      // whether that be by successfully executing, soft failure, hard failure, or expiration.
      if( trx.delay_sec.value == 0 ) { // Do not double bill. Only charge if we have not already charged for the delay.
         const auto& cfg = control.get_global_properties().configuration;
         add_net_usage( static_cast<uint64_t>(cfg.base_per_transaction_net_usage)
                         + static_cast<uint64_t>(config::transaction_id_net_usage) ); // Will exit early if net usage cannot be payed.
      }

      auto first_auth = trx.first_authorizer();

      uint32_t trx_size = 0;
      const auto& cgto = control.mutable_db().create<generated_transaction_object>( [&]( auto& gto ) {
        gto.trx_id      = id;
        gto.payer       = first_auth;
        gto.sender      = account_name(); /// delayed transactions have no sender
        gto.sender_id   = transaction_id_to_sender_id( gto.trx_id );
        gto.published   = control.pending_block_time();
        gto.delay_until = gto.published + delay;
        gto.expiration  = gto.delay_until + fc::seconds(control.get_global_properties().configuration.deferred_trx_expiration_window);
        trx_size = gto.set( trx );
      });

      int64_t ram_delta = (config::billable_size_v<generated_transaction_object> + trx_size);
      add_ram_usage( cgto.payer, ram_delta );
      trace->account_ram_delta = account_delta( cgto.payer, ram_delta );
   }

   void transaction_context::record_transaction( const transaction_id_type& id, fc::time_point_sec expire ) {
      try {
          control.mutable_db().create<transaction_object>([&](transaction_object& transaction) {
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

   void transaction_context::validate_referenced_accounts( const transaction& trx, bool enforce_actor_whitelist_blacklist )const {
      const auto& db = control.db();
      const auto& auth_manager = control.get_authorization_manager();

      for( const auto& a : trx.context_free_actions ) {
         auto* code = db.find<account_object, by_name>(a.account);
         EOS_ASSERT( code != nullptr, transaction_exception,
                     "action's code account '${account}' does not exist", ("account", a.account) );
         EOS_ASSERT( a.authorization.size() == 0, transaction_exception,
                     "context-free actions cannot have authorizations" );
      }

      flat_set<account_name> actors;

      bool one_auth = false;
      for( const auto& a : trx.actions ) {
         auto* code = db.find<account_object, by_name>(a.account);
         EOS_ASSERT( code != nullptr, transaction_exception,
                     "action's code account '${account}' does not exist", ("account", a.account) );
         for( const auto& auth : a.authorization ) {
            one_auth = true;
            auto* actor = db.find<account_object, by_name>(auth.actor);
            EOS_ASSERT( actor  != nullptr, transaction_exception,
                        "action's authorizing actor '${account}' does not exist", ("account", auth.actor) );
            EOS_ASSERT( auth_manager.find_permission(auth) != nullptr, transaction_exception,
                        "action's authorizations include a non-existent permission: ${permission}",
                        ("permission", auth) );
            if( enforce_actor_whitelist_blacklist )
               actors.insert( auth.actor );
         }
      }
      EOS_ASSERT( one_auth, tx_no_auths, "transaction must have at least one authorization" );

      if( enforce_actor_whitelist_blacklist ) {
         control.check_actor_list( actors );
      }
   }


} } /// eosio::chain
