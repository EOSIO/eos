#pragma once

#include <chainbase/chainbase.hpp>
#include <eosio/chain/multi_index_includes.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

   struct by_kv_key;

   struct kv_object : public chainbase::object<kv_object_type, kv_object> {
      OBJECT_CTOR(kv_object, (kv_key)(kv_value))

      id_type     id;
      uint8_t     database_id = 0;
      uint64_t    contract    = 0;
      shared_blob kv_key;
      shared_blob kv_value;
   };

   using kv_index = chainbase::shared_multi_index_container<
         kv_object,
         indexed_by<ordered_unique<tag<by_id>, member<kv_object, kv_object::id_type, &kv_object::id>>,
                    ordered_unique<tag<by_kv_key>,
                                   composite_key<kv_object, member<kv_object, uint8_t, &kv_object::database_id>,
                                                 member<kv_object, uint64_t, &kv_object::contract>,
                                                 member<kv_object, shared_blob, &kv_object::kv_key>,
                                                 member<kv_object, shared_blob, &kv_object::kv_value>>,
                                   composite_key_compare<std::less<uint8_t>, std::less<uint64_t>,
                                                         std::less<shared_blob>, std::less<shared_blob>>>>>;

}} // namespace eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::kv_object, eosio::chain::kv_index)
FC_REFLECT(eosio::chain::kv_object, (database_id)(contract)(kv_key)(kv_value))
