#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/stake_object.hpp>

#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/snapshot.hpp>

#include <set>

#include <cyberway/chaindb/common.hpp>

namespace cyberway { namespace chaindb {
    struct ram_payer_info;
}} // namespace cyberway::chaindb

namespace eosio { namespace chain { 
    
namespace config {
    static constexpr auto _1percent = 100;
    static constexpr auto _100percent = 100 * _1percent;
}

namespace resource_limits {
   using cyberway::chaindb::ram_payer_info;

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

   struct account_resource_limit {
      int64_t used = 0; ///< quantity used in current window
      int64_t available = 0; ///< quantity available in current window (based upon fractional reserve)
      int64_t max = 0; ///< max per window under current congestion
      int64_t staked_virtual_balance = 0;
   };
   
   static auto const cpu_code = symbol(0,"CPU").to_symbol_code();
   static auto const net_code = symbol(0,"NET").to_symbol_code();
   static auto const ram_code = symbol(0,"RAM").to_symbol_code();

   class resource_limits_manager {
      public:
         explicit resource_limits_manager(cyberway::chaindb::chaindb_controller& chaindb)
         :_chaindb(chaindb)
         {
         }

         void add_indices();
         void initialize_database();
         void add_to_snapshot( const snapshot_writer_ptr& snapshot ) const;
         void read_from_snapshot( const snapshot_reader_ptr& snapshot );

         void initialize_account( const account_name& account );
         void set_block_parameters( const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters );

         void update_account_usage( const flat_set<account_name>& accounts, uint32_t ordinal );
         void add_transaction_usage( const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t ordinal, int64_t now = -1 );

         void add_pending_ram_usage( const account_name account, int64_t ram_delta );

         void process_block_usage( uint32_t block_num );

         // accessors
         uint64_t get_virtual_block_cpu_limit() const;
         uint64_t get_virtual_block_net_limit() const;

         uint64_t get_block_cpu_limit() const;
         uint64_t get_block_net_limit() const;

         account_resource_limit get_account_limit_ex(int64_t now, const account_name& account, symbol_code purpose_code);

         int64_t get_account_ram_usage( const account_name& name ) const;
         
         void update_proxied(const ram_payer_info&, int64_t now, symbol_code token_code, const account_name& account, int64_t frame_length, bool force);
         void recall_proxied(const ram_payer_info&, int64_t now, symbol_code token_code, account_name grantor_name, account_name agent_name, int16_t pct);

      private:
         cyberway::chaindb::chaindb_controller& _chaindb;
   };
} } } /// eosio::chain

FC_REFLECT( eosio::chain::resource_limits::ratio, (numerator)(denominator))
FC_REFLECT( eosio::chain::resource_limits::elastic_limit_parameters, (target)(max)(periods)(max_multiplier)(contract_rate)(expand_rate))
FC_REFLECT( eosio::chain::resource_limits::account_resource_limit, (used)(available)(max)(staked_virtual_balance) )

