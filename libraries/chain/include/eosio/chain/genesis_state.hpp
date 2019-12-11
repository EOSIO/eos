#pragma once

#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/chain_snapshot.hpp>
#include <eosio/chain/kv_config.hpp>
#include <eosio/chain/types.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace eosio { namespace chain {

namespace legacy {

   struct snapshot_genesis_state_v3 {
      static constexpr uint32_t minimum_version = 0;
      static constexpr uint32_t maximum_version = 3;
      static_assert(chain_snapshot_header::minimum_compatible_version <= maximum_version, "snapshot_genesis_state_v3 is no longer needed");
      time_point                               initial_timestamp;
      public_key_type                          initial_key;
      chain_config                             initial_configuration;
   };

}

struct genesis_state {
   genesis_state();

   static const string eosio_root_key;

   chain_config   initial_configuration = {
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

   static constexpr kv_config default_initial_kv_configuration = {
      { config::default_max_kv_key_size, config::default_max_kv_value_size, config::default_max_kv_iterators },
      { config::default_max_kv_key_size, config::default_max_kv_value_size, config::default_max_kv_iterators }
   };

   kv_config initial_kv_configuration = default_initial_kv_configuration;

   time_point                               initial_timestamp;
   public_key_type                          initial_key;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;

   friend inline bool operator==( const genesis_state& lhs, const genesis_state& rhs ) {
     return std::tie( lhs.initial_configuration, lhs.initial_timestamp, lhs.initial_key, lhs.initial_kv_configuration )
             == std::tie( rhs.initial_configuration, rhs.initial_timestamp, rhs.initial_key, rhs.initial_kv_configuration );
   };

   friend inline bool operator!=( const genesis_state& lhs, const genesis_state& rhs ) { return !(lhs == rhs); }

   void initialize_from( const legacy::snapshot_genesis_state_v3& legacy ) {
      initial_configuration    = legacy.initial_configuration;
      initial_timestamp        = legacy.initial_timestamp;
      initial_key              = legacy.initial_key;
      initial_kv_configuration = default_initial_kv_configuration;
   }
};

} } // namespace eosio::chain


FC_REFLECT(eosio::chain::genesis_state,
           (initial_timestamp)(initial_key)(initial_configuration)(initial_kv_configuration))

FC_REFLECT(eosio::chain::legacy::snapshot_genesis_state_v3,
           (initial_timestamp)(initial_key)(initial_configuration))
