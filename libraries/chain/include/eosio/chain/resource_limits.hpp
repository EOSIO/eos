#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/snapshot.hpp>
#include <chainbase/chainbase.hpp>
#include <set>

namespace eosio { namespace chain { namespace resource_limits {
   namespace impl {
      template<typename T>
      struct ratio {
         static_assert(std::is_integral<T>::value, "ratios must have integral types");
         T numerator;
         T denominator;

         friend inline bool operator ==( const ratio& lhs, const ratio& rhs ) {
            return std::tie(lhs.numerator, lhs.denominator) == std::tie(rhs.numerator, rhs.denominator);
         }

         friend inline bool operator !=( const ratio& lhs, const ratio& rhs ) {
            return !(lhs == rhs);
         }
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

      friend inline bool operator ==( const elastic_limit_parameters& lhs, const elastic_limit_parameters& rhs ) {
         return std::tie(lhs.target, lhs.max, lhs.periods, lhs.max_multiplier, lhs.contract_rate, lhs.expand_rate)
                  == std::tie(rhs.target, rhs.max, rhs.periods, rhs.max_multiplier, rhs.contract_rate, rhs.expand_rate);
      }

      friend inline bool operator !=( const elastic_limit_parameters& lhs, const elastic_limit_parameters& rhs ) {
         return !(lhs == rhs);
      }
   };

   struct account_resource_limit {
      int64_t used = 0; ///< quantity used in current window
      int64_t available = 0; ///< quantity available in current window (based upon fractional reserve)
      int64_t max = 0; ///< max per window under current congestion
   };

   class resource_limits_manager {
      public:
         explicit resource_limits_manager(chainbase::database& db)
         :_db(db)
         {
         }

         void add_indices();
         void initialize_database();
         void add_to_snapshot( const snapshot_writer_ptr& snapshot ) const;
         void read_from_snapshot( const snapshot_reader_ptr& snapshot );

         void initialize_account( const account_name& account );
         void set_block_parameters( const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters );

         void update_account_usage( const flat_set<account_name>& accounts, uint32_t ordinal );
         void add_transaction_usage( const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t ordinal );

         void add_pending_ram_usage( const account_name account, int64_t ram_delta );
         void verify_account_ram_usage( const account_name accunt )const;

         /// set_account_limits returns true if new ram_bytes limit is more restrictive than the previously set one
         bool set_account_limits( const account_name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight);
         void get_account_limits( const account_name& account, int64_t& ram_bytes, int64_t& net_weight, int64_t& cpu_weight) const;

         void process_account_limit_updates();
         void process_block_usage( uint32_t block_num );

         // accessors
         uint64_t get_virtual_block_cpu_limit() const;
         uint64_t get_virtual_block_net_limit() const;

         uint64_t get_block_cpu_limit() const;
         uint64_t get_block_net_limit() const;

         std::pair<int64_t, bool> get_account_cpu_limit( const account_name& name, uint32_t greylist_limit = config::maximum_elastic_resource_multiplier ) const;
         std::pair<int64_t, bool> get_account_net_limit( const account_name& name, uint32_t greylist_limit = config::maximum_elastic_resource_multiplier ) const;

         std::pair<account_resource_limit, bool> get_account_cpu_limit_ex( const account_name& name, uint32_t greylist_limit = config::maximum_elastic_resource_multiplier ) const;
         std::pair<account_resource_limit, bool> get_account_net_limit_ex( const account_name& name, uint32_t greylist_limit = config::maximum_elastic_resource_multiplier ) const;

         int64_t get_account_ram_usage( const account_name& name ) const;

      private:
         chainbase::database& _db;
   };
} } } /// eosio::chain

FC_REFLECT( eosio::chain::resource_limits::account_resource_limit, (used)(available)(max) )
FC_REFLECT( eosio::chain::resource_limits::ratio, (numerator)(denominator))
FC_REFLECT( eosio::chain::resource_limits::elastic_limit_parameters, (target)(max)(periods)(max_multiplier)(contract_rate)(expand_rate))
