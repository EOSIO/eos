#pragma once
#include <fc/variant.hpp>
#include <eosio/trace_api/trace.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/protocol_feature_activation.hpp>

namespace eosio { namespace trace_api {
   struct block_entry_v0 {
      chain::block_id_type   id;
      uint32_t               number;
      uint64_t               offset;
   };

   struct lib_entry_v0 {
      uint32_t               lib;
   };

   struct trx_id_entry {
      chain::transaction_id_type id;
      uint32_t                   block_num;
   };

   using metadata_log_entry = std::variant<
      block_entry_v0,
      lib_entry_v0,
      trx_id_entry
   >;

}}

FC_REFLECT(eosio::trace_api::block_entry_v0, (id)(number)(offset));
FC_REFLECT(eosio::trace_api::lib_entry_v0, (lib));
FC_REFLECT(eosio::trace_api::trx_id_entry, (id)(block_num));
