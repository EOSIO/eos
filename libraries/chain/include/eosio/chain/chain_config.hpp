#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>

namespace eosio { namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct chain_config_v0 {

   //order must match parameters as ids are used in serialization
   enum {
      max_block_net_usage_id,
      target_block_net_usage_pct_id,
      max_transaction_net_usage_id,
      base_per_transaction_net_usage_id,
      net_usage_leeway_id,
      context_free_discount_net_usage_num_id,
      context_free_discount_net_usage_den_id,
      max_block_cpu_usage_id,
      target_block_cpu_usage_pct_id,
      max_transaction_cpu_usage_id,
      min_transaction_cpu_usage_id,
      max_transaction_lifetime_id,
      deferred_trx_expiration_window_id,
      max_transaction_delay_id,
      max_inline_action_size_id,
      max_inline_action_depth_id,
      max_authority_depth_id,
      PARAMS_COUNT
   };

   uint64_t   max_block_net_usage;                 ///< the maxiumum net usage in instructions for a block
   uint32_t   target_block_net_usage_pct;          ///< the target percent (1% == 100, 100%= 10,000) of maximum net usage; exceeding this triggers congestion handling
   uint32_t   max_transaction_net_usage;           ///< the maximum objectively measured net usage that the chain will allow regardless of account limits
   uint32_t   base_per_transaction_net_usage;      ///< the base amount of net usage billed for a transaction to cover incidentals
   uint32_t   net_usage_leeway;
   uint32_t   context_free_discount_net_usage_num; ///< the numerator for the discount on net usage of context-free data
   uint32_t   context_free_discount_net_usage_den; ///< the denominator for the discount on net usage of context-free data

   uint32_t   max_block_cpu_usage;                 ///< the maxiumum billable cpu usage (in microseconds) for a block
   uint32_t   target_block_cpu_usage_pct;          ///< the target percent (1% == 100, 100%= 10,000) of maximum cpu usage; exceeding this triggers congestion handling
   uint32_t   max_transaction_cpu_usage;           ///< the maximum billable cpu usage (in microseconds) that the chain will allow regardless of account limits
   uint32_t   min_transaction_cpu_usage;           ///< the minimum billable cpu usage (in microseconds) that the chain requires

   uint32_t   max_transaction_lifetime;            ///< the maximum number of seconds that an input transaction's expiration can be ahead of the time of the block in which it is first included
   uint32_t   deferred_trx_expiration_window;      ///< the number of seconds after the time a deferred transaction can first execute until it expires
   uint32_t   max_transaction_delay;               ///< the maximum number of seconds that can be imposed as a delay requirement by authorization checks
   uint32_t   max_inline_action_size;              ///< maximum allowed size (in bytes) of an inline action
   uint16_t   max_inline_action_depth;             ///< recursion depth limit on sending inline actions
   uint16_t   max_authority_depth;                 ///< recursion depth limit for checking if an authority is satisfied

   void validate()const;

   inline const chain_config_v0& v0() const {
      return *this;
   }
   
   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config_v0& c ) {
      return c.log(out) << "\n";
   }

   friend inline bool operator ==( const chain_config_v0& lhs, const chain_config_v0& rhs ) {
      return   std::tie(   lhs.max_block_net_usage,
                           lhs.target_block_net_usage_pct,
                           lhs.max_transaction_net_usage,
                           lhs.base_per_transaction_net_usage,
                           lhs.net_usage_leeway,
                           lhs.context_free_discount_net_usage_num,
                           lhs.context_free_discount_net_usage_den,
                           lhs.max_block_cpu_usage,
                           lhs.target_block_cpu_usage_pct,
                           lhs.max_transaction_cpu_usage,
                           lhs.max_transaction_cpu_usage,
                           lhs.max_transaction_lifetime,
                           lhs.deferred_trx_expiration_window,
                           lhs.max_transaction_delay,
                           lhs.max_inline_action_size,
                           lhs.max_inline_action_depth,
                           lhs.max_authority_depth
                        )
               ==
               std::tie(   rhs.max_block_net_usage,
                           rhs.target_block_net_usage_pct,
                           rhs.max_transaction_net_usage,
                           rhs.base_per_transaction_net_usage,
                           rhs.net_usage_leeway,
                           rhs.context_free_discount_net_usage_num,
                           rhs.context_free_discount_net_usage_den,
                           rhs.max_block_cpu_usage,
                           rhs.target_block_cpu_usage_pct,
                           rhs.max_transaction_cpu_usage,
                           rhs.max_transaction_cpu_usage,
                           rhs.max_transaction_lifetime,
                           rhs.deferred_trx_expiration_window,
                           rhs.max_transaction_delay,
                           rhs.max_inline_action_size,
                           rhs.max_inline_action_depth,
                           rhs.max_authority_depth
                        );
   };

   friend inline bool operator !=( const chain_config_v0& lhs, const chain_config_v0& rhs ) { return !(lhs == rhs); }

protected:
   template<typename Stream>
   Stream& log(Stream& out) const{
      return out << "Max Block Net Usage: " << max_block_net_usage << ", "
                     << "Target Block Net Usage Percent: " << ((double)target_block_net_usage_pct / (double)config::percent_1) << "%, "
                     << "Max Transaction Net Usage: " << max_transaction_net_usage << ", "
                     << "Base Per-Transaction Net Usage: " << base_per_transaction_net_usage << ", "
                     << "Net Usage Leeway: " << net_usage_leeway << ", "
                     << "Context-Free Data Net Usage Discount: " << (double)context_free_discount_net_usage_num * 100.0 / (double)context_free_discount_net_usage_den << "% , "

                     << "Max Block CPU Usage: " << max_block_cpu_usage << ", "
                     << "Target Block CPU Usage Percent: " << ((double)target_block_cpu_usage_pct / (double)config::percent_1) << "%, "
                     << "Max Transaction CPU Usage: " << max_transaction_cpu_usage << ", "
                     << "Min Transaction CPU Usage: " << min_transaction_cpu_usage << ", "

                     << "Max Transaction Lifetime: " << max_transaction_lifetime << ", "
                     << "Deferred Transaction Expiration Window: " << deferred_trx_expiration_window << ", "
                     << "Max Transaction Delay: " << max_transaction_delay << ", "
                     << "Max Inline Action Size: " << max_inline_action_size << ", "
                     << "Max Inline Action Depth: " << max_inline_action_depth << ", "
                     << "Max Authority Depth: " << max_authority_depth;
   }
};

/**
 * @brief v1 Producer-voted blockchain configuration parameters
 *
 * If Adding new parameters create chain_config_v[n] class instead of adding
 * new parameters to v1 or v0. This is needed for snapshots backward compatibility
 */
struct chain_config_v1 : chain_config_v0 {
   using Base = chain_config_v0;

   uint32_t   max_action_return_value_size = config::default_max_action_return_value_size;               ///< size limit for action return value
   
   //order must match parameters as ids are used in serialization
   enum {
     max_action_return_value_size_id = Base::PARAMS_COUNT,
     PARAMS_COUNT
   };

   inline const Base& base() const {
      return static_cast<const Base&>(*this);
   }

   void validate() const;

   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config_v1& c ) {
      return c.log(out) << "\n";
   }

   friend inline bool operator == ( const chain_config_v1& lhs, const chain_config_v1& rhs ) {
      //add v1 parameters comarison here
      return std::tie(lhs.max_action_return_value_size) == std::tie(rhs.max_action_return_value_size) 
          && lhs.base() == rhs.base();
   }

   friend inline bool operator != ( const chain_config_v1& lhs, const chain_config_v1& rhs ) {
      return !(lhs == rhs);
   }

   inline chain_config_v1& operator= (const Base& b) {
      Base::operator= (b);
      return *this;
   }

protected:
   template<typename Stream>
   Stream& log(Stream& out) const{
      return base().log(out) << ", Max Action Return Value Size: " << max_action_return_value_size;
   }
};

class controller;

struct config_entry_validator{
   const controller& control;

   bool operator()(uint32_t id) const;
};

//after adding 1st value to chain_config_v1 change this using to point to v1
using chain_config = chain_config_v1;

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::chain_config_v0,
           (max_block_net_usage)(target_block_net_usage_pct)
           (max_transaction_net_usage)(base_per_transaction_net_usage)(net_usage_leeway)
           (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)

           (max_block_cpu_usage)(target_block_cpu_usage_pct)
           (max_transaction_cpu_usage)(min_transaction_cpu_usage)

           (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
           (max_inline_action_size)(max_inline_action_depth)(max_authority_depth)

)

FC_REFLECT_DERIVED(eosio::chain::chain_config_v1, (eosio::chain::chain_config_v0), 
           (max_action_return_value_size)
)
