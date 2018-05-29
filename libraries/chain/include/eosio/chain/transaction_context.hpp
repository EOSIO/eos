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

         void init_for_implicit_trx( uint64_t initial_net_usage = 0 );

         void init_for_input_trx( uint64_t packed_trx_unprunable_size,
                                  uint64_t packed_trx_prunable_size,
                                  uint32_t num_signatures              );

         void init_for_deferred_trx( fc::time_point published );

         void exec();
         void finalize();
         void squash();

         inline void add_net_usage( uint64_t u ) { net_usage += u; check_net_usage(); }

         void check_net_usage()const;

         void checktime()const;

         void pause_billing_timer();
         void resume_billing_timer();

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

         void validate_cpu_usage_to_bill( int64_t u, bool check_minimum = true )const;

      /// Fields:
      public:

         controller&                   control;
         const signed_transaction&     trx;
         transaction_id_type           id;
         chainbase::database::session  undo_session;
         transaction_trace_ptr         trace;
         fc::time_point                start;

         fc::time_point                published;


         vector<action_receipt>        executed;
         flat_set<account_name>        bill_to_accounts;
         flat_set<account_name>        validate_ram_usage;

         /// the maximum number of virtual CPU instructions of the transaction that can be safely billed to the billable accounts
         uint64_t                      initial_max_billable_cpu = 0;

         fc::microseconds              delay;
         bool                          is_input           = false;
         bool                          apply_context_free = true;

         fc::time_point                deadline = fc::time_point::maximum();
         fc::microseconds              leeway = fc::microseconds(1000);
         int64_t                       billed_cpu_time_us = 0;

      private:
         bool                          is_initialized = false;


         uint64_t                      net_limit = 0;
         bool                          net_limit_due_to_block = true;
         uint64_t                      eager_net_limit = 0;
         uint64_t&                     net_usage; /// reference to trace->net_usage

         fc::microseconds              objective_duration_limit;
         fc::time_point                _deadline = fc::time_point::maximum();
         int64_t                       deadline_exception_code = block_cpu_usage_exceeded::code_value;
         int64_t                       billing_timer_exception_code = block_cpu_usage_exceeded::code_value;
         fc::time_point                pseudo_start;
         fc::microseconds              billed_time;
         fc::microseconds              billing_timer_duration_limit;
   };

} }
