#pragma once

#include <chainbase/chainbase.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/chain/multi_index_includes.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/backing_store.hpp>

namespace eosio { namespace chain {
   class kv_db_config_object : public chainbase::object<kv_db_config_object_type, kv_db_config_object> {
      OBJECT_CTOR(kv_db_config_object)

      id_type            id;
      backing_store_type backing_store = backing_store_type::CHAINBASE;
   };

   using kv_db_config_index = chainbase::shared_multi_index_container<
         kv_db_config_object,
         indexed_by<ordered_unique<tag<by_id>,
                                   BOOST_MULTI_INDEX_MEMBER(kv_db_config_object, kv_db_config_object::id_type, id)>>>;

   struct by_kv_key;

   struct kv_object_view {
      name database_id;
      name contract;
      // TODO: Use non-owning memory
      fc::blob kv_key;
      fc::blob kv_value;
   };

   struct kv_object : public chainbase::object<kv_object_type, kv_object> {
      OBJECT_CTOR(kv_object, (kv_key)(kv_value))

      static constexpr uint32_t minimum_snapshot_version = 4;

      id_type     id;
      name        database_id;
      name        contract;
      shared_blob kv_key;
      shared_blob kv_value;
      name        payer;
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
      // NOTICE: Do not change any of the constants defined here; otherwise it would cause backward concensus compatibility problem.
      static constexpr uint64_t overhead                     = overhead_per_row_per_index_ram_bytes * 2;
      static constexpr uint64_t serialized_shared_blob_size  = 8 + 4; // 8 for vector data 4 for vector size
      static constexpr uint64_t serialized_kv_object_size_exclude_shared_blobs = 32; // derived from sizeof(id_type) + 3* sizeof(name)
      static constexpr uint64_t value = serialized_kv_object_size_exclude_shared_blobs + serialized_shared_blob_size * 2 + overhead; 
   };
} // namespace config

}} // namespace eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::kv_db_config_object, eosio::chain::kv_db_config_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::kv_object, eosio::chain::kv_index)
FC_REFLECT(eosio::chain::kv_db_config_object, (backing_store))
FC_REFLECT(eosio::chain::kv_object_view, (database_id)(contract)(kv_key)(kv_value))
FC_REFLECT(eosio::chain::kv_object, (database_id)(contract)(kv_key)(kv_value)(payer))
