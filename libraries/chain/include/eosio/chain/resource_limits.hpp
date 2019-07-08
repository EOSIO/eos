#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/symbol.hpp>

#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/snapshot.hpp>

#include <set>

#include <eosio/chain/int_arithmetic.hpp>
#include <eosio/chain/chain_config.hpp>

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
   static constexpr ratio constant_rate = ratio{ uint64_t(config::percent_100), uint64_t(config::percent_100) };
   
   
   struct elastic_limit_parameters {
      uint64_t target;           // the desired usage
      uint64_t min;
      uint64_t max;             
      uint32_t periods;          // the number of aggregation periods that contribute to the average usage
      ratio    decrease_rate = constant_rate;   // the rate at which a congested resource contracts its limit
      ratio    increase_rate = constant_rate;     // the rate at which an uncongested resource expands its limits

      void set(uint64_t target_, uint64_t min_, uint64_t max_, uint32_t average_window_ms, uint16_t decrease_pct, uint16_t increase_pct);
   };
   
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

         storage_payer_info get_storage_payer(uint32_t time_slot, account_name owner);

         void initialize_account( const account_name& account, const storage_payer_info& );
         void set_limit_params(const chain_config& chain_cfg);

         void update_account_usage( const flat_set<account_name>& accounts, uint32_t ordinal );

         void add_transaction_usage( const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint64_t ram_usage, fc::time_point pending_block_time, bool validate = true );

         void add_storage_usage(const account_name& account, int64_t delta, uint32_t time_slot, bool);

         void process_block_usage(uint32_t time_slot);

         // accessors
         uint64_t get_virtual_block_limit(resource_id res) const;

         uint64_t get_block_limit(resource_id res, const chain_config& chain_cfg) const;
         
         std::vector<ratio> get_pricelist() const;

         ratio get_resource_usage_by_account_cost_ratio(account_name account, resource_id res) const;
         ratio get_account_stake_ratio(fc::time_point pending_block_time, const account_name& account, bool update_state);

         uint64_t get_account_balance(fc::time_point pending_block_time, const account_name& account, const std::vector<ratio>& prices, bool update_state);

         std::vector<uint64_t> get_account_usage(const account_name& account) const;

      private:
         chaindb_controller& _chaindb;
   };
} } } /// eosio::chain

FC_REFLECT( eosio::chain::resource_limits::ratio, (numerator)(denominator))
FC_REFLECT( eosio::chain::resource_limits::elastic_limit_parameters, (target)(min)(max)(periods)(decrease_rate)(increase_rate))
