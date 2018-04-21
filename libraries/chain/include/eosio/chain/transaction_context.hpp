#pragma once
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {


   class transaction_context {
      public:
         transaction_context( controller& c, 
                              const signed_transaction& t, 
                              const transaction_id_type& trx_id )
         :control(c),
          id(trx_id),
          trx(t),
          undo_session(c.db().start_undo_session(true))
         {
            trace = std::make_shared<transaction_trace>();
            trace->id = id;
            executed.reserve( trx.total_actions() );
         }

         void exec();
         void dispatch_action( const action& a, account_name receiver, bool context_free = false );

         vector<action_receipt>            executed;

         transaction_trace_ptr             trace;

         fc::time_point                    processing_deadline;
         controller&                       control;
         transaction_id_type               id;

         fc::time_point                    published;
         const signed_transaction&         trx;
         uint32_t                          net_usage = 0;
         //const transaction_metadata_ptr&   trx_meta;
         chainbase::database::session      undo_session;
         flat_set<account_name>            bill_to_accounts;
   };

} }
