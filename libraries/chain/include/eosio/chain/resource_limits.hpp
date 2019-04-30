#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/symbol.hpp>

#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/snapshot.hpp>

#include <set>

#include <eosio/chain/int_arithmetic.hpp>

namespace cyberway { namespace chaindb {
    class chaindb_controller;
    struct storage_payer_info;
}} // namespace cyberway::chaindb

namespace eosio { namespace chain {

namespace resource_limits {
   using cyberway::chaindb::storage_payer_info;
   using cyberway::chaindb::chaindb_controller;

   namespace impl {
      template<typename T>
      struct ratio {
         static_assert(std::is_integral<T>::value, "ratios must have integral types");
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

      void validate()const; // throws if the parameters do not satisfy basic sanity checks
   };
   
   struct account_balance {
      uint64_t stake;
      uint64_t ram;
   };

   struct account_storage_usage {
      uint64_t ram_usage;
      uint64_t ram_owned;
      uint64_t storage_usage;
      uint64_t storage_owned;
   };
   
   static auto const cpu_code = symbol(0,"CPU").to_symbol_code();
   static auto const net_code = symbol(0,"NET").to_symbol_code();
   static auto const ram_code = symbol(0,"RAM").to_symbol_code();
   
   using pricelist = std::map<symbol_code, ratio>;
   
   class resource_limits_manager {
      public:
         explicit resource_limits_manager(chaindb_controller& chaindb)
         :_chaindb(chaindb)
         {
         }

         void add_indices();
         void initialize_database();
         void add_to_snapshot( const snapshot_writer_ptr& snapshot ) const;
         void read_from_snapshot( const snapshot_reader_ptr& snapshot );

         storage_payer_info get_storage_payer(account_name owner = account_name());

         void initialize_account( const account_name& account, const storage_payer_info& );
         void set_block_parameters( const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters );

         void update_account_usage( const flat_set<account_name>& accounts, uint32_t ordinal );
         void add_transaction_usage( const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, fc::time_point now );

         void add_storage_usage( const storage_payer_info& );

         void process_block_usage( uint32_t block_num );

         // accessors
         uint64_t get_virtual_block_cpu_limit() const;
         uint64_t get_virtual_block_net_limit() const;

         uint64_t get_block_cpu_limit() const;
         uint64_t get_block_net_limit() const;
         
         pricelist get_pricelist() const;

         ratio get_account_usage_ratio(account_name account, symbol_code resource_code) const;
         ratio get_account_stake_ratio(int64_t now, const account_name& account);

         account_balance get_account_balance(int64_t now, const account_name& account, const pricelist&);

         account_storage_usage get_account_storage_usage(const account_name& account) const;
         std::map<symbol_code, uint64_t> get_account_usage(const account_name& account) const;

      private:
         chaindb_controller& _chaindb;
   };
} } } /// eosio::chain

FC_REFLECT( eosio::chain::resource_limits::ratio, (numerator)(denominator))
FC_REFLECT( eosio::chain::resource_limits::elastic_limit_parameters, (target)(max)(periods)(max_multiplier)(contract_rate)(expand_rate))
