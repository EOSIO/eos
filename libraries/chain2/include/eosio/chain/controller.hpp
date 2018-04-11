#pragma once
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/genesis_state.hpp>
#include <boost/signals2/signal.hpp>

namespace chainbase {
   class database;
}

namespace eosio { namespace chain {

   struct controller_impl;
   using chainbase::database;
   using boost::signals2::signal;

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
         void start_block( block_timestamp_type time );
         // void finalize_block( signing_lambda );
                             
         block_state_ptr             push_block( const signed_block_ptr& b );
         transaction_trace           push_transaction( const signed_transaction& t );
         optional<transaction_trace> push_deferred_transaction();

         const chainbase::database& db()const;

         uint32_t head_block_num();

         signal<void(const block_trace_ptr&)>  applied_block;
         signal<void(const signed_block&)> applied_irreversible_block;

      private:
         std::unique_ptr<controller_impl> my;

   };

} }  /// eosio::chain

FC_REFLECT( eosio::chain::controller::config, 
            (block_log_dir)
            (shared_memory_dir)(shared_memory_size)(read_only)
            (genesis)
            (limits)(wasm_runtime) 
          )
