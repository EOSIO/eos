#pragma once
#include <eosio/chain/types.hpp>
#include <chainbase/chainbase.hpp>

namespace eosio { namespace chain { namespace resource_limits {
   class resource_limits_manager {
      public:
         resource_limits_manager(chainbase::database& db)
         :_db(db)
         {
         }

         void initialize_database();
         void initialize_chain();
         void initialize_account( const account_name& account );

         void add_account_usage( const vector<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t block_num );
         void add_block_usage( uint64_t cpu_usage, uint64_t net_usage, uint32_t block_num );

         void set_account_limits( const account_name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
         void get_account_limits( const account_name& account, int64_t& ram_bytes, int64_t& net_weight, int64_t& cpu_weight) const;

         void process_pending_updates();

      private:
         chainbase::database& _db;
   };
} } } /// eosio::chain
