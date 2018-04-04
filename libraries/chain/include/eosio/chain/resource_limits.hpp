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

         void add_transaction_usage( const vector<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t ordinal );

         void add_account_ram_usage( const account_name account, int64_t ram_delta, const char* use_format = "Unspecified", const fc::variant_object& args = fc::variant_object() );

         void set_account_limits( const account_name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
         void get_account_limits( const account_name& account, int64_t& ram_bytes, int64_t& net_weight, int64_t& cpu_weight) const;

         void process_account_limit_updates();
         void process_block_usage( uint32_t block_num );

         // accessors
         uint64_t get_virtual_block_cpu_limit() const;
         uint64_t get_virtual_block_net_limit() const;

         uint64_t get_block_cpu_limit() const;
         uint64_t get_block_net_limit() const;

         int64_t get_account_cpu_limit( const account_name& name ) const;
         int64_t get_account_net_limit( const account_name& name ) const;

      private:
         chainbase::database& _db;
   };
} } } /// eosio::chain

