#pragma once
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {

   class transaction_context {
      private:
         void init( uint64_t initial_net_usage );

      public:

         transaction_context( controller& c,
                              const signed_transaction& t,
                              const transaction_id_type& trx_id,
                              fc::time_point start = fc::time_point::now() );

         void init_for_implicit_trx( fc::time_point deadline,
                                     uint64_t initial_net_usage = 0
                                     );

         void init_for_input_trx( fc::time_point deadline,
                                  uint64_t packed_trx_unprunable_size,
                                  uint64_t packed_trx_prunable_size,
                                  uint32_t num_signatures              );

         void init_for_deferred_trx( fc::time_point deadline,
                                     fc::time_point published );

         void exec();
         void finalize();
         void squash();

         inline void add_net_usage( uint64_t u ) { net_usage += u; check_net_usage(); }

         void check_net_usage()const;
         void check_time()const;

         void add_ram_usage( account_name account, int64_t ram_delta );

      private:

         friend struct controller_impl;
         friend class apply_context;

         void dispatch_action( action_trace& trace, const action& a, account_name receiver, bool context_free = false, uint32_t recurse_depth = 0 );
         inline void dispatch_action( action_trace& trace, const action& a, bool context_free = false ) {
            dispatch_action(trace, a, a.account, context_free);
         };
         void schedule_transaction();
         void record_transaction( const transaction_id_type& id, fc::time_point_sec expire );

      /// Fields:
      public:

         controller&                   control;
         const signed_transaction&     trx;
         transaction_id_type           id;
         chainbase::database::session  undo_session;
         transaction_trace_ptr         trace;
         fc::time_point                start;

         fc::time_point                published;
         fc::time_point                deadline = fc::time_point::maximum();

         vector<action_receipt>        executed;
         flat_set<account_name>        bill_to_accounts;
         flat_set<account_name>        validate_ram_usage;

         /// the maximum number of network usage bytes the transaction can consume (ignoring what billable accounts can pay and ignoring the remaining usage available in the block)
         uint64_t                      max_net = 0;
         uint64_t                      eager_net_limit = 0;

         /// the maximum number of virtual CPU instructions of the transaction that can be safely billed to the billable accounts
         uint64_t                      initial_max_billable_cpu = 0;

         fc::microseconds              delay;
         bool                          is_input           = false;
         bool                          apply_context_free = true;

         uint32_t                      billed_cpu_time_us = 0;
      private:

         uint64_t&                     net_usage; /// reference to trace->net_usage
         bool                          net_limit_due_to_block = false;
         bool                          cpu_limit_due_to_block = false;
         bool                          is_initialized = false;

   };

} }
