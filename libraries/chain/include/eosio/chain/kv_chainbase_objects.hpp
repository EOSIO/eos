#pragma once

#include <chainbase/chainbase.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/multi_index_includes.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

   struct by_kv_key;

   struct kv_object : public chainbase::object<kv_object_type, kv_object> {
      OBJECT_CTOR(kv_object, (kv_key)(kv_value))

      static constexpr uint32_t minimum_snapshot_version = 4;

      id_type     id;
      name        database_id;
      name        contract;
      shared_blob kv_key;
      shared_blob kv_value;
   };

   using kv_index = chainbase::shared_multi_index_container<
         kv_object,
         indexed_by<ordered_unique<tag<by_id>, member<kv_object, kv_object::id_type, &kv_object::id>>,
                    ordered_unique<tag<by_kv_key>,
                                   composite_key<kv_object, member<kv_object, name, &kv_object::database_id>,
                                                 member<kv_object, name, &kv_object::contract>,
                                                 member<kv_object, shared_blob, &kv_object::kv_key>>,
                                   composite_key_compare<std::less<name>, std::less<name>, unsigned_blob_less>>>>;

namespace config {
   template<>
   struct billable_size<kv_object> {
      static constexpr uint64_t overhead = overhead_per_row_per_index_ram_bytes * 2;
      static constexpr uint64_t value = 24 + (8 + 4) * 2 + overhead; ///< 24 for fixed fields 8 for vector data 4 for vector size
   };
} // namespace config

}} // namespace eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::kv_object, eosio::chain::kv_index)
FC_REFLECT(eosio::chain::kv_object, (database_id)(contract)(kv_key)(kv_value))
