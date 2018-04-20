#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/exceptions.hpp>


namespace eosio { namespace chain {

   void transaction_context::exec() {

      auto& trx = trx_meta->trx;

      EOS_ASSERT( trx.max_kcpu_usage.value < UINT32_MAX / 1024UL, transaction_exception, "declared max_kcpu_usage overflows when expanded to max cpu usage" );
      EOS_ASSERT( trx.max_net_usage_words.value < UINT32_MAX / 8UL, transaction_exception, "declared max_net_usage_words overflows when expanded to max net usage" );



      control.record_transaction( trx_meta ); /// checks for dupes
      control.validate_tapos( trx ); 
      control.validate_referenced_accounts( trx );
      control.validate_expiration( trx );

      auto usage = control.validate_net_usage( trx_meta );

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


} } /// eosio::chain
