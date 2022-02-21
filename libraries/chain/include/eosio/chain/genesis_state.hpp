#pragma once

#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/wasm_config.hpp>
#include <eosio/chain/types.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace eosio { namespace chain {

struct genesis_state {
   genesis_state();

   static const string eosio_root_key;

   chain_config_v0   initial_configuration = {
      .max_block_net_usage                  = config::default_max_block_net_usage,
      .target_block_net_usage_pct           = config::default_target_block_net_usage_pct,
      .max_transaction_net_usage            = config::default_max_transaction_net_usage,
      .base_per_transaction_net_usage       = config::default_base_per_transaction_net_usage,
      .net_usage_leeway                     = config::default_net_usage_leeway,
      .context_free_discount_net_usage_num  = config::default_context_free_discount_net_usage_num,
      .context_free_discount_net_usage_den  = config::default_context_free_discount_net_usage_den,

      .max_block_cpu_usage                  = config::default_max_block_cpu_usage,
      .target_block_cpu_usage_pct           = config::default_target_block_cpu_usage_pct,
      .max_transaction_cpu_usage            = config::default_max_transaction_cpu_usage,
      .min_transaction_cpu_usage            = config::default_min_transaction_cpu_usage,

      .max_transaction_lifetime             = config::default_max_trx_lifetime,
      .deferred_trx_expiration_window       = config::default_deferred_trx_expiration_window,
      .max_transaction_delay                = config::default_max_trx_delay,
      .max_inline_action_size               = config::default_max_inline_action_size,
      .max_inline_action_depth              = config::default_max_inline_action_depth,
      .max_authority_depth                  = config::default_max_auth_depth,
   };

   static constexpr wasm_config default_initial_wasm_configuration {
      .max_mutable_global_bytes = config::default_max_wasm_mutable_global_bytes,
      .max_table_elements       = config::default_max_wasm_table_elements,
      .max_section_elements     = config::default_max_wasm_section_elements,
      .max_linear_memory_init   = config::default_max_wasm_linear_memory_init,
      .max_func_local_bytes     = config::default_max_wasm_func_local_bytes,
      .max_nested_structures    = config::default_max_wasm_nested_structures,
      .max_symbol_bytes         = config::default_max_wasm_symbol_bytes,
      .max_module_bytes         = config::default_max_wasm_module_bytes,
      .max_code_bytes           = config::default_max_wasm_code_bytes,
      .max_pages                = config::default_max_wasm_pages,
      .max_call_depth           = config::default_max_wasm_call_depth
   };

   time_point                               initial_timestamp;
   public_key_type                          initial_key;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;

   friend inline bool operator==( const genesis_state& lhs, const genesis_state& rhs ) {
      return std::tie( lhs.initial_configuration, lhs.initial_timestamp, lhs.initial_key )
               == std::tie( rhs.initial_configuration, rhs.initial_timestamp, rhs.initial_key );
   };

   friend inline bool operator!=( const genesis_state& lhs, const genesis_state& rhs ) { return !(lhs == rhs); }

};

} } // namespace eosio::chain

// @swap initial_timestamp initial_key initial_configuration
FC_REFLECT(eosio::chain::genesis_state,
           (initial_timestamp)(initial_key)(initial_configuration))
