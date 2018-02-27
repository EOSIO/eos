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

   uint32_t   target_block_size;
   uint32_t   max_block_size;

   uint32_t   target_block_acts_per_scope;
   uint32_t   max_block_acts_per_scope;

   uint32_t   target_block_acts; ///< regardless of the amount of parallelism, this defines target compute time per block
   uint32_t   max_block_acts; ///< regardless of the amount of parallelism, this maximum compute time per block

   uint64_t   real_threads; ///< the number of real threads the producers are using

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
      return out << "Target Block Size: " << c.target_block_size << ", "
                 << "Max Block Size: " << c.max_block_size << ", " 
                 << "Target Block Acts Per Scope: " << c.target_block_acts_per_scope << ", " 
                 << "Max Block Acts Per Scope: " << c.max_block_acts_per_scope << ", " 
                 << "Target Block Acts: " << c.target_block_acts << ", " 
                 << "Max Block Acts: " << c.max_block_acts << ", " 
                 << "Real Threads: " << c.real_threads << ", " 
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
           (target_block_size)
           (max_block_size)
           (target_block_acts_per_scope)
           (max_block_acts_per_scope)
           (target_block_acts)
           (max_block_acts)
           (real_threads)
           (max_storage_size)
           (max_transaction_lifetime)(max_authority_depth)(max_transaction_exec_time)
           (max_inline_depth)(max_inline_action_size)(max_generated_transaction_size) )
