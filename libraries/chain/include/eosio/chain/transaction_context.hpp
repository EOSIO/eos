#pragma once
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
#include <signal.h>

namespace eosio { namespace chain {

   struct provided_bandwith {
       provided_bandwith() = default;

       void confirm(account_name provider);

       bool is_confirmed() const {return confirmed_;}

       int64_t get_net_limit() const {return net_limit_;}
       int64_t get_cpu_limit() const {return cpu_limit_;}

       void set_net_limit(int64_t net_limit);
       void set_cpu_limit(int64_t cpu_limit);

       account_name get_provider() const {return provider_;}

   private:
       
       void verify_limits_not_confirmed();

       int64_t net_limit_ = 0;
       int64_t cpu_limit_ = 0;
       bool confirmed_ = false;
       account_name provider_;
   };


   struct bandwith_request_result {
       std::map<account_name, provided_bandwith> bandwith;
       uint64_t used_net;
       uint64_t used_cpu;
   };

   struct deadline_timer {
         deadline_timer();
         ~deadline_timer();

         void start(fc::time_point tp);
         void stop();

         static volatile sig_atomic_t expired;
      private:
         static void timer_expired(int);
         static bool initialized;
   };

   class transaction_context {
      private:
         void init( uint64_t initial_net_usage);

      public:

         transaction_context( controller& c,
                              const signed_transaction& t,
                              const transaction_id_type& trx_id,
                              fc::time_point start = fc::time_point::now() );

         void init_for_implicit_trx( uint64_t initial_net_usage = 0 );

         void init_for_input_trx( uint64_t packed_trx_unprunable_size,
                                  uint64_t packed_trx_prunable_size,
                                  uint32_t num_signatures,
                                  bool skip_recording);

         void init_for_deferred_trx( fc::time_point published );

         void exec();
         void finalize();
         void squash();
         void undo();
         void validate_bw_usage();

         inline void add_net_usage( uint64_t u ) { net_usage += u; check_net_usage(); }
         inline void add_cpu_usage( uint64_t u ) { billed_cpu_time_us += u;}

         uint64_t get_net_usage() const {return net_usage;}
         uint64_t get_cpu_usage() const {return billed_cpu_time_us;}

         void check_net_usage()const;

         void checktime()const;

         void pause_billing_timer();
         void resume_billing_timer();

         uint32_t update_billed_cpu_time( fc::time_point now );

         std::tuple<int64_t, int64_t, bool, bool> max_bandwidth_billed_accounts_can_pay( bool force_elastic_limits = false )const;

         uint64_t get_provided_net_limit(account_name account) const;

         uint64_t get_provided_cpu_limit(account_name account) const;

         std::map<account_name, provided_bandwith> get_provided_bandwith() const {return provided_bandwith_;}

         bool is_provided_bandwith_confirmed(account_name account) const;

         void set_provided_bandwith(std::map<account_name, provided_bandwith>&& bandwith);

         void set_provided_bandwith_limits(account_name account, uint64_t net_limit, uint64_t cpu_limit);

         void confirm_provided_bandwith_limits(account_name account, account_name provider);

      private:

         friend struct controller_impl;
         friend class apply_context;

         void add_ram_usage( account_name account, int64_t ram_delta );

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
         // TODO: removed by CyberWay
         // optional<chainbase::database::session>  undo_session;
         optional<chaindb_session>     chaindb_undo_session;
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
         bool                          can_subjectively_fail = true;

         fc::time_point                deadline = fc::time_point::maximum();
         fc::microseconds              leeway = fc::microseconds(3000);
         int64_t                       billed_cpu_time_us = 0;
         bool                          explicit_billed_cpu_time = false;

      private:
         bool                          is_initialized = false;


         uint64_t                      net_limit = 0;
         bool                          net_limit_due_to_block = true;
         bool                          net_limit_due_to_greylist = false;
         uint64_t                      eager_net_limit = 0;
         uint64_t&                     net_usage; /// reference to trace->net_usage

         bool                          cpu_limit_due_to_greylist = false;

         fc::microseconds              initial_objective_duration_limit;
         fc::microseconds              objective_duration_limit;
         fc::time_point                _deadline = fc::time_point::maximum();
         int64_t                       deadline_exception_code = block_cpu_usage_exceeded::code_value;
         int64_t                       billing_timer_exception_code = block_cpu_usage_exceeded::code_value;
         fc::time_point                pseudo_start;
         fc::microseconds              billed_time;
         fc::microseconds              billing_timer_duration_limit;

         deadline_timer                _deadline_timer;

         std::map<account_name, provided_bandwith> provided_bandwith_;
   };

} }
