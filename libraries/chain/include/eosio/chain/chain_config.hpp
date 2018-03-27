/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct chain_config {

   uint64_t   real_threads; ///< the number of real threads the producers are using

   uint32_t   base_per_transaction_net_usage; ///< the base amount of net usage billed for a transaction to cover incidentals
   uint32_t   base_per_transaction_cpu_usage; ///< the base amount of cpu usage billed for a transaction to cover incidentals
   uint32_t   base_per_action_cpu_usage;      ///< the base amount of cpu usage billed for an action to cover incidentals
   uint32_t   base_setcode_cpu_usage;         ///< the base amount of cpu usage billed for a setcode action to cover compilation/etc
   uint32_t   per_signature_cpu_usage;        ///< the cpu usage billed for every signature on a transaction

   uint64_t   max_storage_size;
   uint32_t   max_transaction_lifetime;
   uint16_t   max_authority_depth;
   uint32_t   max_transaction_exec_time;
   uint16_t   max_inline_depth;
   uint32_t   max_inline_action_size;
   uint32_t   max_generated_transaction_size;

   static chain_config get_median_values( vector<chain_config> votes );

   template<typename Stream>
   friend Stream& operator << ( Stream& out, const chain_config& c ) {
      return out << "Real Threads: " << c.real_threads << ", "
                 << "Base Per-Transaction Net Usage: " << c.base_per_transaction_net_usage << ", "
                 << "Base Per-Transaction CPU Usage: " << c.base_per_transaction_cpu_usage << ", "
                 << "Base Per-Action CPU Usage: " << c.base_per_action_cpu_usage << ", "
                 << "Base Setcode CPU Usage: " << c.base_setcode_cpu_usage << ", "
                 << "Per-Signature CPU Usage: " << c.per_signature_cpu_usage << ", "
                 << "Max Storage Size: " << c.max_storage_size << ", "
                 << "Max Transaction Lifetime: " << c.max_transaction_lifetime << ", " 
                 << "Max Authority Depth: " << c.max_authority_depth << ", " 
                 << "Max Transaction Exec Time: " << c.max_transaction_exec_time << ", " 
                 << "Max Inline Depth: " << c.max_inline_depth << ", " 
                 << "Max Inline Action Size: " << c.max_inline_action_size << ", " 
                 << "Max Generated Transaction Size: " << c.max_generated_transaction_size << "\n";
   }
};

       bool operator==(const chain_config& a, const chain_config& b);
inline bool operator!=(const chain_config& a, const chain_config& b) { return !(a == b); }

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::chain_config, 
           (real_threads)
           (base_per_transaction_net_usage)
           (base_per_transaction_cpu_usage)
           (base_per_action_cpu_usage)
           (base_setcode_cpu_usage)
           (per_signature_cpu_usage)
           (max_storage_size)
           (max_transaction_lifetime)(max_authority_depth)(max_transaction_exec_time)
           (max_inline_depth)(max_inline_action_size)(max_generated_transaction_size) )
