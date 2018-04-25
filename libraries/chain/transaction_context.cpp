#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>


namespace eosio { namespace chain {

   void transaction_context::exec() {

      EOS_ASSERT( trx.max_kcpu_usage.value < UINT32_MAX / 1024UL, transaction_exception, "declared max_kcpu_usage overflows when expanded to max cpu usage" );
      EOS_ASSERT( trx.max_net_usage_words.value < UINT32_MAX / 8UL, transaction_exception, "declared max_net_usage_words overflows when expanded to max net usage" );

      control.validate_tapos( trx );
      control.validate_referenced_accounts( trx );

      if( is_input ) { /// signed transaction from user vs implicit transaction scheduled by contracts
         control.validate_expiration( trx );
         record_transaction( id, trx.expiration ); /// checks for dupes
      }

      if( trx.max_kcpu_usage.value != 0 ) {
         max_cpu = uint64_t(trx.max_kcpu_usage.value)*1024;
      } else {
         max_cpu = uint64_t(-1);
      }

      if( apply_context_free ) {
         for( const auto& act : trx.context_free_actions ) {
            dispatch_action( act, act.account, true );
         }
      }

      if( delay == fc::microseconds() ) {
         for( const auto& act : trx.actions ) {
           dispatch_action( act, act.account, false );
         }
      } else {
          schedule_transaction();
      }

      for( const auto& act : trx.actions ) {
         for( const auto& auth : act.authorization )
            bill_to_accounts.insert( auth.actor );
      }

      FC_ASSERT( trace->cpu_usage <= max_cpu, "transaction consumed too many CPU cycles" );

      auto& rl       = control.get_mutable_resource_limits_manager();

      vector<account_name> bta( bill_to_accounts.begin(), bill_to_accounts.end() );
      rl.add_transaction_usage( bta, trace->cpu_usage, net_usage, block_timestamp_type(control.pending_block_time()).slot );

   }

   void transaction_context::squash() {
      undo_session.squash();
   }

   void transaction_context::dispatch_action( const action& a, account_name receiver, bool context_free ) {
      apply_context  acontext( control, a, trx );

      acontext.id                   = id;
      acontext.context_free         = context_free;
      acontext.receiver             = receiver;
      acontext.processing_deadline  = deadline;
      acontext.published_time       = published;
      acontext.max_cpu              = max_cpu - trace->cpu_usage;
      acontext.exec();

      fc::move_append(executed, move(acontext.executed) );

      trace->cpu_usage += acontext.trace.total_inline_cpu_usage;
      trace->action_traces.emplace_back( move(acontext.trace) );
   }

   void transaction_context::schedule_transaction() {
      auto first_auth = trx.first_authorizor();

      const auto& cgto = control.db().create<generated_transaction_object>( [&]( auto& gto ) {
        gto.trx_id      = id;
        gto.payer       = first_auth;
        gto.sender      = account_name(); /// auto-boxed trxs have no sender
        gto.sender_id   = transaction_id_to_sender_id( gto.trx_id );
        gto.published   = control.pending_block_time();
        gto.expiration  = gto.published + delay + fc::seconds(60*10); // TODO: make 10 minutes configurable by system
        gto.delay_until = gto.published + delay;
        gto.set( trx );
      });

      control.get_mutable_resource_limits_manager().add_pending_account_ram_usage(cgto.payer,
               (config::billable_size_v<generated_transaction_object> + cgto.packed_trx.size()));
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
