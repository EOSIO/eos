#pragma once
#include <eosio/chain/controller.hpp>

namespace eosio { namespace chain {


   class transaction_context {
      public:
         transaction_context( controller& c, const transaction_metadata_ptr& trx ) 
         :control(c),
          trx_meta(trx),
          undo_session(c.db().start_undo_session(true))
         {
            executed.reserve( trx_meta->total_actions() );
         }

         void exec() {
            control.record_transaction( trx_meta );

            for( const auto& act : trx_meta->trx.context_free_actions ) {
               dispatch_action( act, act.account, true );
            }

            for( const auto& act : trx_meta->trx.actions ) {
               dispatch_action( act, act.account, false );
            }

            undo_session.squash();
         }

         void dispatch_action( const action& a, account_name receiver, bool context_free = false ) {
            action_receipt r;
            r.receiver        = receiver;
            r.act             = a;
            r.global_sequence = control.next_global_sequence();
            r.recv_sequence   = control.next_recv_sequence( receiver );

            for( const auto& auth : a.authorization ) {
               r.auth_sequence[auth.actor] = control.next_auth_sequence( auth.actor );
            }

            executed.emplace_back( move(r) );
         }

         vector<action_receipt>            executed;

         controller&                       control;
         const transaction_metadata_ptr&   trx_meta;
         chainbase::database::session      undo_session;
   };

} }
