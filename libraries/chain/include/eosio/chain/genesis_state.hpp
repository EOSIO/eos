#pragma once

#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/chain_snapshot.hpp>
#include <eosio/chain/wasm_config.hpp>
#include <eosio/chain/kv_config.hpp>
#include <eosio/chain/types.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace eosio { namespace chain {

struct genesis_state_v0 {
   genesis_state_v0();
   time_point                               initial_timestamp;
   public_key_type                          initial_key;

   static constexpr chain_config_v0 default_initial_configuration = {
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

   void validate() const { initial_configuration.validate(); }

   chain_config_v0   initial_configuration = default_initial_configuration;
   friend inline bool operator==( const genesis_state_v0& lhs, const genesis_state_v0& rhs ) {
      return std::tie( lhs.initial_configuration, lhs.initial_timestamp, lhs.initial_key )
               == std::tie( rhs.initial_configuration, rhs.initial_timestamp, rhs.initial_key );
   };
};

struct genesis_state_chain_config_v1 : chain_config_v0 {
   std::optional<uint32_t> max_action_return_value_size;
   friend inline bool operator==( const genesis_state_chain_config_v1& lhs, const genesis_state_chain_config_v1& rhs ) {
     return std::tie( lhs.v0(), lhs.max_action_return_value_size )
         == std::tie( rhs.v0(), rhs.max_action_return_value_size );
   };
};

struct genesis_state_v1 {
   genesis_state_v1();

   genesis_state_chain_config_v1 initial_configuration = {
      { genesis_state_v0::default_initial_configuration },
      std::nullopt
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
   std::vector<digest_type>                 initial_protocol_features;

   // protocol feature specific initialization
   std::optional<kv_config> initial_kv_configuration;
   std::optional<wasm_config> initial_wasm_configuration;

   void validate() const;

   friend inline bool operator==( const genesis_state_v1& lhs, const genesis_state_v1& rhs ) {
     return std::tie( lhs.initial_configuration, lhs.initial_timestamp, lhs.initial_key, lhs.initial_protocol_features,
                      lhs.initial_kv_configuration, lhs.initial_wasm_configuration )
         == std::tie( rhs.initial_configuration, rhs.initial_timestamp, rhs.initial_key, rhs.initial_protocol_features,
                      rhs.initial_kv_configuration, rhs.initial_wasm_configuration);
   };

   friend inline bool operator!=( const genesis_state_v1& lhs, const genesis_state_v1& rhs ) { return !(lhs == rhs); }
};

struct genesis_state {
   static const string eosio_root_key;

   genesis_state();
   genesis_state(genesis_state_v0&&);
   genesis_state(genesis_state_v1&&);

   const chain_config_v0&   initial_configuration() const { return std::visit([](const auto& v) -> const chain_config_v0& { return (v.initial_configuration); }, _impl); }
   chain_config_v0&         initial_configuration() { return std::visit([](auto& v) -> chain_config_v0& { return (v.initial_configuration); }, _impl); }
   const time_point&        initial_timestamp() const { return std::visit([](const auto& v) -> decltype(auto) { return (v.initial_timestamp); }, _impl); }
   time_point&              initial_timestamp() { return std::visit([](auto& v) -> decltype(auto) { return (v.initial_timestamp); }, _impl); }
   const public_key_type&   initial_key() const { return std::visit([](const auto& v) -> decltype(auto) { return (v.initial_key); }, _impl); }
   public_key_type&         initial_key() { return std::visit([](auto& v) -> decltype(auto) { return (v.initial_key); }, _impl); }
   const std::vector<digest_type>* initial_protocol_features() const {
      return std::visit(overloaded{[](const genesis_state_v0&) -> const std::vector<digest_type>* {
                                      return nullptr;
                                   },
                                   [](const auto& v){
                                      return &v.initial_protocol_features;
                                   }}, _impl);
   }
   std::vector<digest_type>* initial_protocol_features() {
      return std::visit(overloaded{[](genesis_state_v0&) -> std::vector<digest_type>* {
                                      return nullptr;
                                   },
                                   [](auto& v){
                                      return &v.initial_protocol_features;
                                   }}, _impl);
   }

   // Configuration for specific protocol features
   const uint32_t* max_action_return_value_size() const;
   const kv_config* initial_kv_configuration() const;
   const wasm_config* initial_wasm_configuration() const;

   void validate() const {
      std::visit([](const auto& v) { v.validate(); }, _impl);
   }

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;

   void initialize_from( const genesis_state_v0& legacy ) {
      _impl = legacy;
   }

   genesis_state_v0 v0() const {
      return std::get<genesis_state_v0>(_impl);
   }

   friend inline bool operator==( const genesis_state& lhs, const genesis_state& rhs ) {
      return lhs._impl == rhs._impl;
   };

   friend inline bool operator!=( const genesis_state& lhs, const genesis_state& rhs ) { return !(lhs == rhs); }

   std::variant<genesis_state_v0, genesis_state_v1> _impl;
};

void from_variant(const fc::variant& v, genesis_state& gs);
void to_variant(const genesis_state& gs, fc::variant&);

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::genesis_state_v0,
           (initial_timestamp)(initial_key)(initial_configuration))

FC_REFLECT_DERIVED(eosio::chain::genesis_state_chain_config_v1, (eosio::chain::chain_config_v0),
           (max_action_return_value_size))

// @swap initial_timestamp initial_key initial_configuration
FC_REFLECT(eosio::chain::genesis_state_v1,
           (initial_timestamp)(initial_key)(initial_configuration)(initial_protocol_features)(initial_kv_configuration)(initial_wasm_configuration))

FC_REFLECT(eosio::chain::genesis_state, (_impl))
