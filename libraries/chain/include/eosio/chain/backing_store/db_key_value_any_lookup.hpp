#pragma once

#include <chain_kv/chain_kv.hpp>
#include <eosio/chain/backing_store/db_key_value_iter_store.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace backing_store {
   struct db_context;

   struct key_bundle {
      bytes          composite_key;
      rocksdb::Slice key;           // points to all of composite_key
   };

   struct prefix_bundle {
      bytes          composite_key;
      rocksdb::Slice key;           // points to all of composite_key
      rocksdb::Slice prefix;        // points to front prefix (maybe include type) of composite_key
   };

   template<typename Key>
   struct value_bundle : Key {
      value_bundle(Key&& key, std::shared_ptr<const bytes>&& v) : Key(std::move(key)), value(std::move(v)) {}
      std::shared_ptr<const bytes> value;
   };

   using kv_bundle = value_bundle<key_bundle>;
   using pv_bundle = value_bundle<prefix_bundle>;

   struct db_key_value_any_lookup {
      db_key_value_any_lookup(db_context& c, b1::chain_kv::view& v) : parent(c), view(v) {}
      virtual ~db_key_value_any_lookup() {}

      static key_bundle get_slice(name scope, name table);
      static key_bundle get_table_end_slice(name scope, name table);
      void add_table_if_needed(name scope, name table, account_name payer);
      void remove_table_if_empty(name scope, name table);
      template<typename IterStore>
      int32_t get_end_iter(name code, name scope, name table, IterStore& iter_store) {
         const auto table_key = get_table_end_slice(scope, table);
         std::shared_ptr<const bytes> value = view.get(code.to_uint64_t(), table_key.key);
         if (!value) {
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         return table_ei;
      }

      db_context&              parent;
      b1::chain_kv::view&      view;
      static constexpr int64_t table_overhead = config::billable_size_v<table_id_object>;
      static constexpr int64_t overhead = config::billable_size_v<key_value_object>;
   };

}}} // ns eosio::chain::backing_store
