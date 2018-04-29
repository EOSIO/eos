#pragma once
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {


   class transaction_context {
      private:
         // Common private constructor
         transaction_context( controller& c,
                              transaction_trace_ptr& trace_ptr,
                              const signed_transaction& t,
                              const transaction_id_type& trx_id,
                              fc::time_point deadline,
                              fc::time_point published,
                              uint64_t initial_net_usage,
                              uint64_t initial_cpu_usage         );

         static uint64_t calculate_initial_net_usage( const controller& c,
                                                      const signed_transaction& t,
                                                      uint64_t packed_trx_billable_size );

      public:
         // For deferred transactions
         transaction_context( transaction_trace_ptr& trace_ptr,
                              controller& c,
                              const signed_transaction& t,
                              const transaction_id_type& trx_id,
                              fc::time_point deadline,
                              fc::time_point published           );

         // For implicit or input transactions
         transaction_context( transaction_trace_ptr& trace_ptr,
                              controller& c,
                              const signed_transaction& t,
                              const transaction_id_type& trx_id,
                              fc::time_point deadline,
                              bool is_implicit,
                              uint64_t packed_trx_billable_size,
                              uint64_t initial_cpu_usage = 0      );

         void exec();
         void squash();

         inline void add_net_usage( uint64_t u ) { net_usage += u; check_net_usage(); }
         inline void add_cpu_usage( uint64_t u ) { cpu_usage += u; check_cpu_usage(); }
         inline void add_cpu_usage_and_check_time( uint64_t u ) { check_time(); cpu_usage += u; check_cpu_usage(); }

         void check_net_usage()const;
         void check_cpu_usage()const;
         void check_time()const;

      private:

         void dispatch_action( const action& a, account_name receiver, bool context_free = false );
         inline void dispatch_action( const action& a, bool context_free = false ) {
            dispatch_action(a, a.account, context_free);
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
         fc::time_point                deadline;
         fc::time_point                published;

         vector<action_receipt>        executed;
         flat_set<account_name>        bill_to_accounts;
         uint64_t                      max_net = 0;   /// the maximum number of network usage bytes the transaction can consume
         uint64_t                      max_cpu = 0;   /// the maximum number of CPU instructions the transaction may consume
         fc::microseconds              delay;
         bool                          is_input           = false;
         bool                          apply_context_free = true;

      private:

         uint64_t&                     net_usage; /// reference to trace->net_usage
         uint64_t&                     cpu_usage; /// reference to trace->cpu_usage

   };

} }
