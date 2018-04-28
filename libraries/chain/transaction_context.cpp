#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>

namespace eosio { namespace chain {

   void transaction_context::exec() {

      trx.validate();
      control.validate_tapos( trx );
      control.validate_referenced_accounts( trx );

      if( is_input ) { /// signed transaction from user vs implicit transaction scheduled by contracts
         control.validate_expiration( trx );
         record_transaction( id, trx.expiration ); /// checks for dupes
      }
      max_cpu = control.get_global_properties().configuration.max_transaction_cpu_usage;
      // TODO: reduce max_cpu to the minimum of the amount that the paying accounts can afford to pay
      uint64_t trx_specified_cpu_usage_limit = uint64_t(trx.max_kcpu_usage.value)*1024; // overflow checked in transaction_header::validate()
      if( trx_specified_cpu_usage_limit > 0 )
         max_cpu = std::min( max_cpu, trx_specified_cpu_usage_limit );

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

      trace->cpu_usage = ((trace->cpu_usage + 1023)/1024)*1024; // Round up to nearest multiple of 1024

      EOS_ASSERT( trace->cpu_usage <= max_cpu, tx_resource_exhausted,
                  "cpu usage of transaction is too high: ${actual_net_usage} > ${cpu_usage_limit}",
                  ("actual_net_usage", trace->cpu_usage)("cpu_usage_limit", max_cpu) );

      auto& rl = control.get_mutable_resource_limits_manager();

      flat_set<account_name> bta( bill_to_accounts.begin(), bill_to_accounts.end() );
      FC_ASSERT( net_usage % 8 == 0, "net_usage must be a multiple of word size (8)" );
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
        gto.delay_until = gto.published + delay;
        gto.expiration  = gto.delay_until + fc::milliseconds(config::deferred_trx_expiration_window_ms);
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
