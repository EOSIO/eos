#pragma once
#include "privileged.h"
#include "serialize.hpp"
#include "types.h"

namespace eosio {

   struct blockchain_parameters {
      uint64_t max_block_net_usage;
      uint32_t target_block_net_usage_pct;
      uint32_t max_transaction_net_usage;
      uint32_t base_per_transaction_net_usage;
      uint32_t net_usage_leeway;
      uint32_t context_free_discount_net_usage_num;
      uint32_t context_free_discount_net_usage_den;

      uint32_t max_block_cpu_usage;
      uint32_t target_block_cpu_usage_pct;
      uint32_t max_transaction_cpu_usage;
      uint32_t min_transaction_cpu_usage;

      uint32_t max_transaction_lifetime;
      uint32_t deferred_trx_expiration_window;
      uint32_t max_transaction_delay;
      uint32_t max_inline_action_size;
      uint16_t max_inline_action_depth;
      uint16_t max_authority_depth;
      uint32_t max_generated_transaction_count;

      EOSLIB_SERIALIZE( blockchain_parameters,
                        (max_block_net_usage)(target_block_net_usage_pct)
                        (max_transaction_net_usage)(base_per_transaction_net_usage)(net_usage_leeway)
                        (context_free_discount_net_usage_num)(context_free_discount_net_usage_den)

                        (max_block_cpu_usage)(target_block_cpu_usage_pct)
                        (max_transaction_cpu_usage)(min_transaction_cpu_usage)

                        (max_transaction_lifetime)(deferred_trx_expiration_window)(max_transaction_delay)
                        (max_inline_action_size)(max_inline_action_depth)
                        (max_authority_depth)(max_generated_transaction_count)
      )
   };

   void set_blockchain_parameters(const eosio::blockchain_parameters& params);

   void get_blockchain_parameters(eosio::blockchain_parameters& params);

   struct producer_key {
      account_name producer_name;
      public_key   block_signing_key;

      friend bool operator < ( const producer_key& a, const producer_key& b ) {
         return a.producer_name < b.producer_name;
      }

      EOSLIB_SERIALIZE( producer_key, (producer_name)(block_signing_key) )
   };

}
