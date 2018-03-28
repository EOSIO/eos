#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <chainbase/chainbase.hpp>

namespace eosio { namespace chain { namespace resource_limits {
   class resource_limits_manager {
      public:
         explicit resource_limits_manager(chainbase::database& db)
         :_db(db)
         {
         }

         void initialize_database();
         void initialize_chain();
         void initialize_account( const account_name& account );

         void add_account_usage( const vector<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t ordinal );
         void add_account_ram_usage( const account_name account, int64_t ram_delta, const char* use_format = "Unspecified", const fc::variant_object& args = fc::variant_object() );
         void add_block_usage( uint64_t cpu_usage, uint64_t net_usage, uint32_t ordinal );

         void set_account_limits( const account_name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
         void get_account_limits( const account_name& account, int64_t& ram_bytes, int64_t& net_weight, int64_t& cpu_weight) const;

         void process_pending_updates();

      private:
         chainbase::database& _db;
   };
} } } /// eosio::chain

