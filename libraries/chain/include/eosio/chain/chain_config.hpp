/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
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
struct chain_config {

   uint32_t   base_per_transaction_net_usage; ///< the base amount of net usage billed for a transaction to cover incidentals
   uint32_t   base_per_transaction_cpu_usage; ///< the base amount of cpu usage billed for a transaction to cover incidentals
   uint32_t   base_per_action_cpu_usage;      ///< the base amount of cpu usage billed for an action to cover incidentals
   uint32_t   base_setcode_cpu_usage;         ///< the base amount of cpu usage billed for a setcode action to cover compilation/etc
   uint32_t   per_signature_cpu_usage;        ///< the cpu usage billed for every signature on a transaction
   uint32_t   per_lock_net_usage;             ///< the net usage billed for every lock on a transaction to cover overhead in the block shards

   uint64_t   context_free_discount_cpu_usage_num;  ///< the numerator for the discount on cpu usage for CFA's
   uint64_t   context_free_discount_cpu_usage_den;  ///< the denominator for the discount on cpu usage for CFA's

   uint32_t   max_transaction_cpu_usage;      ///< the maximum objectively measured cpu usage that the chain will allow regardless of account limits
   uint32_t   max_transaction_net_usage;      ///< the maximum objectively measured net usage that the chain will allow regardless of account limits

   uint64_t   max_block_cpu_usage;            ///< the maxiumum cpu usage in instructions for a block
   uint32_t   target_block_cpu_usage_pct;     ///< the target percent (1% == 100, 100%= 10,000) of maximum cpu usage; exceeding this triggers congestion handling
   uint64_t   max_block_net_usage;            ///< the maxiumum net usage in instructions for a block
   uint32_t   target_block_net_usage_pct;     ///< the target percent (1% == 100, 100%= 10,000) of maximum net usage; exceeding this triggers congestion handling

   uint32_t   max_transaction_lifetime;
   uint32_t   max_transaction_exec_time;
   uint16_t   max_authority_depth;
   uint16_t   max_inline_depth;
   uint32_t   max_inline_action_size;
   uint32_t   max_generated_transaction_count;

   static chain_config get_median_values( vector<chain_config> votes );

   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config& c ) {
      return out << "Base Per-Transaction Net Usage: " << c.base_per_transaction_net_usage << ", "
                 << "Base Per-Transaction CPU Usage: " << c.base_per_transaction_cpu_usage << ", "
                 << "Base Per-Action CPU Usage: " << c.base_per_action_cpu_usage << ", "
                 << "Base Setcode CPU Usage: " << c.base_setcode_cpu_usage << ", "
                 << "Per-Signature CPU Usage: " << c.per_signature_cpu_usage << ", "
                 << "Per-Lock NET Usage: " << c.per_lock_net_usage << ", "
                 << "Context-Free Action CPU Usage Discount: " << (double)c.context_free_discount_cpu_usage_num * 100.0 / (double)c.context_free_discount_cpu_usage_den << "% , "
                 << "Max Transaction CPU Usage: " << c.max_transaction_cpu_usage << ", "
                 << "Max Transaction Net Usage: " << c.max_transaction_net_usage << ", "
                 << "Max Block CPU Usage: " << c.max_block_cpu_usage << ", "
                 << "Target Block CPU Usage Percent: " << c.target_block_cpu_usage_pct << ", "
                 << "Max Block NET Usage: " << c.max_block_net_usage << ", "
                 << "Target Block NET Usage Percent: " << ((double)c.target_block_net_usage_pct / (double)config::percent_1) << "%, "
                 << "Max Transaction Lifetime: " << ((double)c.max_transaction_lifetime / (double)config::percent_1) << "%, "
                 << "Max Authority Depth: " << c.max_authority_depth << ", "
                 << "Max Inline Depth: " << c.max_inline_depth << ", "
                 << "Max Inline Action Size: " << c.max_inline_action_size << ", "
                 << "Max Generated Transaction Count: " << c.max_generated_transaction_count << "\n";
   }
};

       bool operator==(const chain_config& a, const chain_config& b);
inline bool operator!=(const chain_config& a, const chain_config& b) { return !(a == b); }

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::chain_config,
           (base_per_transaction_net_usage)
           (base_per_transaction_cpu_usage)
           (base_per_action_cpu_usage)
           (base_setcode_cpu_usage)
           (per_signature_cpu_usage)(per_lock_net_usage)
           (context_free_discount_cpu_usage_num)(context_free_discount_cpu_usage_den)
           (max_transaction_cpu_usage)(max_transaction_net_usage)
           (max_block_cpu_usage)(target_block_cpu_usage_pct)
           (max_block_net_usage)(target_block_net_usage_pct)
           (max_transaction_lifetime)(max_transaction_exec_time)(max_authority_depth)
           (max_inline_depth)(max_inline_action_size)(max_generated_transaction_count)
)
