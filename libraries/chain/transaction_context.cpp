#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>

namespace eosio { namespace chain {

   transaction_context::transaction_context( controller& c,
                                             transaction_trace_ptr& trace_ptr,
                                             const signed_transaction& t,
                                             const transaction_id_type& trx_id,
                                             fc::time_point d,
                                             fc::time_point p,
                                             uint64_t initial_net_usage,
                                             uint64_t initial_cpu_usage         )
   :control(c)
   ,trx(t)
   ,id(trx_id)
   ,undo_session(c.db().start_undo_session(true))
   ,trace(std::make_shared<transaction_trace>())
   ,deadline(d)
   ,published(p)
   ,net_usage(trace->net_usage)
   ,cpu_usage(trace->cpu_usage)
   {
      trace->id = id;
      trace_ptr = trace;
      executed.reserve( trx.total_actions() );

      // Record accounts to be billed for network and CPU usage
      uint64_t determine_payers_cpu_cost = 0;
      for( const auto& act : trx.actions ) {
         for( const auto& auth : act.authorization ) {
            bill_to_accounts.insert( auth.actor );
            determine_payers_cpu_cost += config::determine_payers_cpu_overhead_per_authorization;
         }
      }
      validate_ram_usage.reserve( bill_to_accounts.size() );

      // Calculate network and CPU usage limits and initial usage:

      // Start with limits set in dynamic configuration
      const auto& cfg = control.get_global_properties().configuration;
      max_net = cfg.max_transaction_net_usage;
      max_cpu = cfg.max_transaction_cpu_usage;

      // Potentially lower limits to what is optionally set in the transaction header
      uint64_t trx_specified_net_usage_limit = static_cast<uint64_t>(trx.max_net_usage_words.value)*8;
      if( trx_specified_net_usage_limit > 0 )
         max_net = std::min( max_net, trx_specified_net_usage_limit );
      uint64_t trx_specified_cpu_usage_limit = static_cast<uint64_t>(trx.max_kcpu_usage.value)*1024;
      if( trx_specified_cpu_usage_limit > 0 )
         max_cpu = std::min( max_cpu, trx_specified_cpu_usage_limit );

      // Initial billing for network usage
      if( initial_net_usage > 0 )
         add_net_usage( initial_net_usage );

      // Initial billing for CPU usage known to be soon consumed
      add_cpu_usage( initial_cpu_usage
                     + static_cast<uint64_t>(cfg.base_per_transaction_cpu_usage)
                     + determine_payers_cpu_cost
                     + bill_to_accounts.size() * config::resource_processing_cpu_overhead_per_billed_account );
      // Fails early if current CPU usage is already greater than the current limit (which may still go lower).

      // Update usage values of accounts to reflect new time
      auto& rl = control.get_mutable_resource_limits_manager();
      rl.add_transaction_usage( bill_to_accounts, 0, 0, block_timestamp_type(control.pending_block_time()).slot );

      // Lower limits to what the billed accounts can afford to pay
      max_net = std::min( max_net, rl.get_block_net_limit() );
      max_cpu = std::min( max_cpu, rl.get_block_cpu_limit() );
      for( const auto& a : bill_to_accounts ) {
         auto net_limit = rl.get_account_net_limit(a);
         if( net_limit >= 0 )
            max_net = std::min( max_net, static_cast<uint64_t>(net_limit) ); // reduce max_net to the amount the account is able to pay
         auto cpu_limit = rl.get_account_cpu_limit(a);
         if( cpu_limit >= 0 )
            max_cpu = std::min( max_cpu, static_cast<uint64_t>(cpu_limit) ); // reduce max_cpu to the amount the account is able to pay
      }

      // Round down network and CPU usage limits so that comparison to actual usage is more efficient
      max_net = (max_net/8)*8;       // Round down to nearest multiple of word size (8 bytes)
      max_cpu = (max_cpu/1024)*1024; // Round down to nearest multiple of 1024
      if( initial_net_usage > 0 )
         check_net_usage();
      check_cpu_usage(); // Fail early if current CPU usage is already greater than the calculated limit
   }

   uint64_t transaction_context::calculate_initial_net_usage( const controller& c,
                                                              const signed_transaction& t,
                                                              uint64_t packed_trx_billable_size ) {
      const auto& cfg = c.get_global_properties().configuration;
      uint64_t initial_net_usage = static_cast<uint64_t>(cfg.base_per_transaction_net_usage) + packed_trx_billable_size;
      if( t.delay_sec.value > 0 ) {
          // If delayed, also charge ahead of time for the additional net usage needed to retire the delayed transaction
          // whether that be by successfully executing, soft failure, hard failure, or expiration.
         initial_net_usage += static_cast<uint64_t>(cfg.base_per_transaction_net_usage)
                               + static_cast<uint64_t>(config::transaction_id_net_usage);
      }
      return initial_net_usage;
   }

   transaction_context::transaction_context( transaction_trace_ptr& trace_ptr,
                                             controller& c,
                                             const signed_transaction& t,
                                             const transaction_id_type& trx_id,
                                             fc::time_point d,
                                             fc::time_point p                   )
   :transaction_context( c, trace_ptr, t, trx_id, d, p, 0, 0 )
   {
      trace->scheduled = true;
      is_input = false;
      apply_context_free = false;
   }

   transaction_context::transaction_context( transaction_trace_ptr& trace_ptr,
                                             controller& c,
                                             const signed_transaction& t,
                                             const transaction_id_type& trx_id,
                                             fc::time_point d,
                                             bool is_implicit,
                                             uint64_t packed_trx_billable_size,
                                             uint64_t initial_cpu_usage         )
   :transaction_context( c, trace_ptr, t, trx_id, d, c.pending_block_time(),
                         calculate_initial_net_usage(c, t, packed_trx_billable_size), initial_cpu_usage )
   {
      trace->scheduled = false;
      is_input = !is_implicit;
   }

   void transaction_context::exec() {
      //trx.validate(); // Not needed anymore since overflow is prevented by using uint64_t instead of uint32_t
      control.validate_tapos( trx );
      control.validate_referenced_accounts( trx );

      if( is_input ) { /// signed transaction from user rather than a deferred transaction
         control.validate_expiration( trx );
         record_transaction( id, trx.expiration ); /// checks for dupes
      }

      if( apply_context_free ) {
         for( const auto& act : trx.context_free_actions ) {
            dispatch_action( act, true );
         }
      }

      if( delay == fc::microseconds() ) {
         for( const auto& act : trx.actions ) {
            dispatch_action( act );
         }
      } else {
         schedule_transaction();
      }

      add_cpu_usage( validate_ram_usage.size() * config::ram_usage_validation_overhead_per_account );

      auto& rl = control.get_mutable_resource_limits_manager();
      for( auto a : validate_ram_usage ) {
         rl.verify_account_ram_usage( a );
      }

      net_usage = ((net_usage + 7)/8)*8; // Round up to nearest multiple of word size (8 bytes)
      cpu_usage = ((cpu_usage + 1023)/1024)*1024; // Round up to nearest multiple of 1024
      control.get_mutable_resource_limits_manager()
             .add_transaction_usage( bill_to_accounts, cpu_usage, net_usage,
                                     block_timestamp_type(control.pending_block_time()).slot );
   }

   void transaction_context::squash() {
      undo_session.squash();
   }

   void transaction_context::check_net_usage()const {
      EOS_ASSERT( BOOST_LIKELY(net_usage <= max_net), tx_resource_exhausted,
                  "net usage of transaction is too high: ${actual_net_usage} > ${net_usage_limit}",
                  ("actual_net_usage", net_usage)("net_usage_limit", max_net) );
   }

   void transaction_context::check_cpu_usage()const {
      EOS_ASSERT( BOOST_LIKELY(cpu_usage <= max_cpu), tx_resource_exhausted,
                  "cpu usage of transaction is too high: ${actual_net_usage} > ${cpu_usage_limit}",
                  ("actual_net_usage", cpu_usage)("cpu_usage_limit", max_cpu) );
   }

   void transaction_context::check_time()const {
      if( BOOST_UNLIKELY(fc::time_point::now() > deadline) ) {
         wlog( "deadline passed" );
         throw checktime_exceeded();
      }
   }

   void transaction_context::add_ram_usage( account_name account, int64_t ram_delta ) {
      auto& rl = control.get_mutable_resource_limits_manager();
      rl.add_pending_ram_usage( account, ram_delta );
      if( ram_delta > 0 ) {
         validate_ram_usage.insert( account );
      }
   }

   void transaction_context::dispatch_action( const action& a, account_name receiver, bool context_free ) {
      apply_context  acontext( control, *this, a );
      acontext.context_free = context_free;
      acontext.receiver     = receiver;
      acontext.exec();

      fc::move_append(executed, move(acontext.executed) );

      trace->action_traces.emplace_back( move(acontext.trace) );
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
      } catch ( ... ) {
          EOS_ASSERT( false, transaction_exception,
                     "duplicate transaction ${id}", ("id", id ) );
      }
   } /// record_transaction


} } /// eosio::chain
