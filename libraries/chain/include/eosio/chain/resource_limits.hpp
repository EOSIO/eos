#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/stake_object.hpp>

#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/snapshot.hpp>

#include <chainbase/chainbase.hpp>
#include <set>

#include <cyberway/chaindb/common.hpp>



namespace eosio { namespace chain { 
    
namespace config {
    static constexpr auto _1percent = 100;
    static constexpr auto _100percent = 100 * _1percent;
}

namespace resource_limits {
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
   };

   class resource_limits_manager {
      public:
         explicit resource_limits_manager(chainbase::database& db, cyberway::chaindb::chaindb_controller& chaindb)
         :_db(db), _chaindb(chaindb)
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

         int64_t get_account_cpu_limit( const account_name& name, bool elastic = true) const;
         int64_t get_account_net_limit( const account_name& name, bool elastic = true) const;

         account_resource_limit get_account_cpu_limit_ex( const account_name& name, bool elastic = true) const;
         account_resource_limit get_account_net_limit_ex( const account_name& name, bool elastic = true) const;

         int64_t get_account_ram_usage( const account_name& name ) const;
         
         void update_proxied(int64_t now, symbol_code purpose_code, symbol_code token_code, const account_name& account, int64_t frame_length, bool force);
         void recall_proxied(int64_t now, symbol_code purpose_code, symbol_code token_code, account_name grantor_name, account_name agent_name, int16_t pct);

      private:
         chainbase::database& _db;
         cyberway::chaindb::chaindb_controller& _chaindb;
         
         using agents_idx_t = decltype(_db.get_mutable_index<stake_agent_index>().indices().get<stake_agent_object::by_key>());
         using grants_idx_t = decltype(_db.get_mutable_index<stake_grant_index>().indices().get<stake_grant_object::by_key>());
         
         static auto agent_key(symbol_code purpose_code, symbol_code token_code, const account_name& agent_name) {
             return boost::make_tuple(purpose_code, token_code, agent_name);
         }
         static auto grant_key(symbol_code purpose_code, symbol_code token_code, const account_name& grantor_name, const account_name& agent_name = account_name()) {
             return boost::make_tuple(purpose_code, token_code, grantor_name, agent_name);
         }
         
         static const stake_agent_object* get_agent(symbol_code purpose_code, symbol_code token_code, const agents_idx_t& agents_idx, const account_name& agent_name) {
            auto agent = agents_idx.find(agent_key(purpose_code, token_code, agent_name));
            EOS_ASSERT(agent != agents_idx.end(), transaction_exception, "agent doesn't exist");
            return &(*agent); 
         }
         
         int64_t recall_proxied_traversal(symbol_code purpose_code, symbol_code token_code,
            const agents_idx_t& agents_idx, const grants_idx_t& grants_idx, 
            const account_name& agent_name, int64_t share, int16_t break_fee);
        
         void update_proxied_traversal(int64_t now, symbol_code purpose_code, symbol_code token_code, 
            const agents_idx_t& agents_idx, const grants_idx_t& grants_idx,
            const stake_agent_object* agent, int64_t frame_length, bool force);
         

   };
} } } /// eosio::chain

FC_REFLECT( eosio::chain::resource_limits::ratio, (numerator)(denominator))
FC_REFLECT( eosio::chain::resource_limits::elastic_limit_parameters, (target)(max)(periods)(max_multiplier)(contract_rate)(expand_rate))
FC_REFLECT( eosio::chain::resource_limits::account_resource_limit, (used)(available)(max) )

