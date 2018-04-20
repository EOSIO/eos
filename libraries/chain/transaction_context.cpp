#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>


namespace eosio { namespace chain {

   void transaction_context::exec() {
      control.record_transaction( trx_meta );
      control.validate_tapos( trx_meta->trx );
      control.validate_referenced_accounts( trx_meta->trx );
      control.validate_expiration( trx_meta->trx );

      for( const auto& act : trx_meta->trx.context_free_actions ) {
         dispatch_action( act, act.account, true );
      }

      for( const auto& act : trx_meta->trx.actions ) {
         dispatch_action( act, act.account, false );
      }

      undo_session.squash();
   }

   void transaction_context::dispatch_action( const action& a, account_name receiver, bool context_free ) {
      apply_context  acontext( control, a, *trx_meta );
      acontext.context_free = context_free;
      acontext.receiver     = receiver;
      acontext.processing_deadline = processing_deadline;
      acontext.exec();

      fc::move_append(executed, move(acontext.executed) );

      trace->cpu_usage += acontext.trace.total_inline_cpu_usage;
      trace->action_traces.emplace_back( move(acontext.trace) );
   }


} }
