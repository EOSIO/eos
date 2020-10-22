#pragma once

#include <b1/chain_kv/chain_kv.hpp>
#include <eosio/chain/backing_store/db_key_value_iter_store.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/types.hpp>
#include <b1/session/shared_bytes.hpp>
#include <b1/session/session.hpp>

namespace eosio { namespace chain { namespace backing_store {
   struct db_context;
   using end_of_prefix = db_key_value_format::end_of_prefix;

   struct key_bundle {
      key_bundle(const b1::chain_kv::bytes& ck, name code);

      eosio::session::shared_bytes full_key;
   };

   struct prefix_bundle {
      prefix_bundle(const b1::chain_kv::bytes& composite_key, end_of_prefix prefix_end, name code);

      eosio::session::shared_bytes full_key;
      eosio::session::shared_bytes prefix_key;
   };

   template<typename Key>
   struct value_bundle : Key {
      value_bundle(Key&& key, std::optional<eosio::session::shared_bytes>&& v) : Key(std::move(key)), value(std::move(v)) {}
      value_bundle(const Key& key, std::optional<eosio::session::shared_bytes>&& v) : Key(key), value(std::move(v)) {}
      std::optional<eosio::session::shared_bytes> value;
   };

   using kv_bundle = value_bundle<key_bundle>;
   using pv_bundle = value_bundle<prefix_bundle>;

   struct db_key_value_any_lookup {
      using session_type = eosio::session::session<eosio::session::session<eosio::session::rocksdb_t>>;
      using shared_bytes = eosio::session::shared_bytes;

      db_key_value_any_lookup(db_context& c, session_type& session) : parent(c), current_session(session) {}

      static key_bundle get_slice(name code, name scope, name table);
      static key_bundle get_table_end_slice(name code, name scope, name table);
      void add_table_if_needed(const shared_bytes& key, account_name payer);
      void remove_table_if_empty(const shared_bytes& key);
      template<typename IterStore>
      int32_t get_end_iter(name code, name scope, name table, IterStore& iter_store) {
         const auto table_key = get_table_end_slice(code, scope, table);
         auto value = current_session.read(table_key.full_key);
         if (!value) {
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         return table_ei;
      }

      bool match_prefix(const shared_bytes& shorter, const shared_bytes& longer);
      bool match_prefix(const shared_bytes& shorter, const session_type::iterator& iter);

      bool match(const shared_bytes& lhs, const shared_bytes& rhs);
      bool match(const shared_bytes& lhs, const session_type::iterator& iter);

      db_context&               parent;
      session_type&             current_session;
      static constexpr int64_t  table_overhead = config::billable_size_v<table_id_object>;
      static constexpr int64_t  overhead = config::billable_size_v<key_value_object>;
      // this is used for any value that just needs something in it to distinguish it from the invalid value
      static const shared_bytes useless_value;
   };

}}} // ns eosio::chain::backing_store
