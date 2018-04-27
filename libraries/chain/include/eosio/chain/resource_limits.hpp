#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <chainbase/chainbase.hpp>

namespace eosio { namespace chain { namespace resource_limits {
   namespace impl {
      template<typename T>
      struct ratio {
         T numerator;
         T denominator;
      };
   }

   using ratio = impl::ratio<uint64_t>;

   struct elastic_limit_parameters {
      uint64_t target;           // the desired usage
      uint64_t max;              // the maximum usage
      uint32_t periods;          // the number of aggregation periods that contribute to the average usage

      uint32_t max_multiplier;   // the multiplier by which virtual space can oversell usage when uncongested
      ratio    contract_rate;    // the rate at which a congested resource contracts its limit
      ratio    expand_rate;       // the rate at which an uncongested resource expands its limits
   };

   class resource_limits_manager {
      public:
         explicit resource_limits_manager(chainbase::database& db)
         :_db(db)
         {
         }

         void initialize_database();
         void initialize_chain();
         void initialize_account( const account_name& account );
         void set_block_parameters( const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters );

         void add_transaction_usage( const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t ordinal );


         void add_pending_account_ram_usage( const account_name account, int64_t ram_delta );
         void synchronize_account_ram_usage( );

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

