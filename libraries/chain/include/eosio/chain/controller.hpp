#pragma once
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/genesis_state.hpp>
#include <boost/signals2/signal.hpp>

namespace chainbase {
   class database;
}


namespace eosio { namespace chain {

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
               fc::microseconds     max_push_block_us = fc::microseconds(-1);
               fc::microseconds     max_push_transaction_us = fc::microseconds(-1);
               fc::microseconds     max_deferred_transactions_us = fc::microseconds(-1);
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

         vector<transaction_metadata_ptr> abort_block();

         transaction_trace_ptr push_transaction( const transaction_metadata_ptr& trx  = transaction_metadata_ptr() );

         /**
          * Attempt to execute a specific transaction in our deferred trx database
          *
          */
         transaction_trace_ptr push_scheduled_transaction( const transaction_id_type& scheduled );

         /**
          * Attempt to execute the oldest unexecuted deferred transaction
          *
          * @return nullptr if there is nothing pending
          */
         transaction_trace_ptr push_next_scheduled_transaction();

         void finalize_block();
         void sign_block( std::function<signature_type( const digest_type& )> signer_callback );
         void commit_block();
         void log_irreversible_blocks();
         void pop_block();
                             
         void push_block( const signed_block_ptr& b );

         chainbase::database& db()const;

         uint32_t   head_block_num()const;
         time_point head_block_time()const;

         block_state_ptr head_block_state()const;
         block_state_ptr pending_block_state()const;

         uint64_t next_global_sequence();
         uint64_t next_recv_sequence( account_name receiver );
         uint64_t next_auth_sequence( account_name actor );
         void     record_transaction( const transaction_metadata_ptr& trx );

         const account_object&                 get_account( account_name n )const;
         const global_property_object&         get_global_properties()const;
         const dynamic_global_property_object& get_dynamic_global_properties()const;
         const permission_object&              get_permission( const permission_level& level )const;
         const resource_limits_manager&        get_resource_limits_manager()const;
         resource_limits_manager&              get_mutable_resource_limits_manager();

         block_id_type        head_block_id()const;
         account_name         head_block_producer()const;
         const block_header&  head_block_header()const;

         const producer_schedule_type& active_producers()const;
         const producer_schedule_type& pending_producers()const;

         uint32_t last_irreversible_block_num() const;

         signed_block_ptr fetch_block_by_number( uint32_t block_num )const;
         signed_block_ptr fetch_block_by_id( block_id_type id )const;

         /**
          * @param actions - the actions to check authorization across
          * @param provided_keys - the set of public keys which have authorized the transaction
          * @param allow_unused_signatures - true if method should not assert on unused signatures
          * @param provided_accounts - the set of accounts which have authorized the transaction (presumed to be owner)
          *
          * @return fc::microseconds set to the max delay that this authorization requires to complete
          */
         fc::microseconds check_authorization( const vector<action>& actions,
                                               const flat_set<public_key_type>& provided_keys,
                                               bool                             allow_unused_signatures = false,
                                               flat_set<account_name>           provided_accounts = flat_set<account_name>(),
                                               flat_set<permission_level>       provided_levels = flat_set<permission_level>()
                                             )const;

         /**
          * @param account - the account owner of the permission
          * @param permission - the permission name to check for authorization
          * @param provided_keys - a set of public keys
          *
          * @return true if the provided keys are sufficient to authorize the account permission
          */
         bool check_authorization( account_name account, permission_name permission,
                                   flat_set<public_key_type> provided_keys,
                                   bool allow_unused_signatures )const;

         void set_active_producers( const producer_schedule_type& sch );

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
         void apply_block( const signed_block_ptr& b );

   };

} }  /// eosio::chain

FC_REFLECT( eosio::chain::controller::config::runtime_limits, (max_push_block_us)(max_push_transaction_us)(max_deferred_transactions_us) )
FC_REFLECT( eosio::chain::controller::config, 
            (block_log_dir)
            (shared_memory_dir)(shared_memory_size)(read_only)
            (genesis)
            (limits)(wasm_runtime) 
          )
