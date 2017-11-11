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
   share_type producer_pay;
   uint32_t   max_block_size;
   uint64_t   max_storage_size;
   uint32_t   max_transaction_lifetime;
   uint16_t   max_authority_depth;
   uint32_t   max_transaction_exec_time;
   uint16_t   max_inline_depth;
   uint32_t   max_inline_action_size;
   uint32_t   max_generated_transaction_size;

   static chain_config get_median_values( vector<chain_config> votes );
};

       bool operator==(const chain_config& a, const chain_config& b);
inline bool operator!=(const chain_config& a, const chain_config& b) { return !(a == b); }

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::chain_config, 
           (target_block_size)(producer_pay)
           (max_block_size)(max_storage_size)
           (max_transaction_lifetime)(max_authority_depth)(max_transaction_exec_time)
           (max_inline_depth)(max_inline_action_size)(max_generated_transaction_size) )
