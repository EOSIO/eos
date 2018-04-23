#pragma once
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/genesis_state.hpp>
#include <boost/signals2/signal.hpp>

namespace chainbase {
   class database;
}


namespace eosio { namespace chain {

   class authorization_manager;

   namespace resource_limits {
      class resource_limits_manager;
   };

   struct controller_impl;
   using chainbase::database;
   using boost::signals2::signal;

   class dynamic_global_property_object;
   class global_property_object;
   class permission_object;
   class account_object;
   using resource_limits::resource_limits_manager;
   using apply_handler = std::function<void(apply_context&)>;

   class controller {
      public:
         struct config {
            struct runtime_limits {
               fc::microseconds     max_push_block_us = fc::microseconds(100000);
               fc::microseconds     max_push_transaction_us = fc::microseconds(1000'000);
               fc::microseconds     max_deferred_transactions_us = fc::microseconds(100000);
            };

            path         block_log_dir       =  chain::config::default_block_log_dir;
            path         shared_memory_dir   =  chain::config::default_shared_memory_dir;
            uint64_t     shared_memory_size  =  chain::config::default_shared_memory_size;
            bool         read_only           =  false;

            genesis_state                  genesis;
            runtime_limits                 limits;
            wasm_interface::vm_type        wasm_runtime = chain::config::default_wasm_runtime;
         };


         controller( const config& cfg );
         ~controller();

         void startup();

         /**
          * Starts a new pending block session upon which new transactions can
          * be pushed.
          */
         void start_block( block_timestamp_type time = block_timestamp_type() );

         void  abort_block();

         /**
          *  These transactions were previously pushed by have since been unapplied, recalling push_transaction
          *  with the transaction_metadata_ptr will remove them from this map whether it fails or succeeds.
          *
          *  @return map of the hash of a signed transaction (with context free data) to a transaction metadata
          */
         const map<digest_type, transaction_metadata_ptr>&  unapplied_transactions()const;


         /**
          *  
          */
         void push_transaction( const transaction_metadata_ptr& trx = transaction_metadata_ptr(),
                                fc::time_point deadline = fc::time_point::maximum() );

         void push_unapplied_transaction( fc::time_point deadline = fc::time_point::maximum() );

         transaction_trace_ptr sync_push( const transaction_metadata_ptr& trx );

         /**
          * Attempt to execute a specific transaction in our deferred trx database
          *
          */
         void push_scheduled_transaction( const transaction_id_type& scheduled,
                                                           fc::time_point deadline = fc::time_point::maximum() );

         /**
          * Attempt to execute the oldest unexecuted deferred transaction
          *
          * @return nullptr if there is nothing pending
          */
         void push_next_scheduled_transaction( fc::time_point deadline = fc::time_point::maximum() );

         void finalize_block();
         void sign_block( const std::function<signature_type( const digest_type& )>& signer_callback );
         void commit_block();
         void log_irreversible_blocks();
         void pop_block();

         void push_block( const signed_block_ptr& b );

         chainbase::database& db()const;

         uint32_t   head_block_num()const;

         time_point head_block_time()const;
         time_point pending_block_time()const;

         block_state_ptr head_block_state()const;
         block_state_ptr pending_block_state()const;

         const account_object&                 get_account( account_name n )const;
         const global_property_object&         get_global_properties()const;
         const dynamic_global_property_object& get_dynamic_global_properties()const;
         const permission_object&              get_permission( const permission_level& level )const;
         const resource_limits_manager&        get_resource_limits_manager()const;
         resource_limits_manager&              get_mutable_resource_limits_manager();
         const authorization_manager&          get_authorization_manager()const;
         authorization_manager&                get_mutable_authorization_manager();

         block_id_type        head_block_id()const;
         account_name         head_block_producer()const;
         const block_header&  head_block_header()const;

         const producer_schedule_type& active_producers()const;
         const producer_schedule_type& pending_producers()const;

         uint32_t last_irreversible_block_num() const;

         signed_block_ptr fetch_block_by_number( uint32_t block_num )const;
         signed_block_ptr fetch_block_by_id( block_id_type id )const;

         void validate_referenced_accounts( const transaction& t )const;
         void validate_expiration( const transaction& t )const;
         void validate_tapos( const transaction& t )const;
         uint64_t validate_net_usage( const transaction_metadata_ptr& trx )const;

         void set_active_producers( const producer_schedule_type& sch );

         signal<void(const block_state_ptr&)>          accepted_block_header;
         signal<void(const block_state_ptr&)>          accepted_block;
         signal<void(const block_state_ptr&)>          irreversible_block;
         signal<void(const transaction_metadata_ptr&)> accepted_transaction;
         signal<void(const transaction_trace_ptr&)>    applied_transaction;

         /*
         signal<void()>                                  pre_apply_block;
         signal<void()>                                  post_apply_block;
         signal<void()>                                  abort_apply_block;
         signal<void(const transaction_metadata_ptr&)>   pre_apply_transaction;
         signal<void(const transaction_trace_ptr&)>      post_apply_transaction;
         signal<void(const transaction_trace_ptr&)>  pre_apply_action;
         signal<void(const transaction_trace_ptr&)>  post_apply_action;
         */

         const apply_handler* find_apply_handler( account_name contract, scope_name scope, action_name act )const;
         wasm_interface& get_wasm_interface();

      private:
         std::unique_ptr<controller_impl> my;

   };

} }  /// eosio::chain

FC_REFLECT( eosio::chain::controller::config::runtime_limits, (max_push_block_us)(max_push_transaction_us)(max_deferred_transactions_us) )
FC_REFLECT( eosio::chain::controller::config,
            (block_log_dir)
            (shared_memory_dir)(shared_memory_size)(read_only)
            (genesis)
            (limits)(wasm_runtime)
          )
