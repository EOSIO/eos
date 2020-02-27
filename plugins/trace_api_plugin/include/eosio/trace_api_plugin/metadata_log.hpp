#pragma once
#include <fc/variant.hpp>
#include <eosio/trace_api_plugin/trace.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/protocol_feature_activation.hpp>

namespace eosio { namespace trace_api_plugin {
   struct block_entry_v0 {
      chain::block_id_type   id;
      uint64_t               number;
      uint64_t               offset;
   };

   struct lib_entry_v0 {
      uint64_t               lib;
   };

   using metadata_log_entry = fc::variant<
      block_entry_v0,
      lib_entry_v0
   >;

}}
