#pragma once

#include <b1/chain_kv/chain_kv.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>
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

   inline eosio::session::shared_bytes make_useless_value() {
      const char null = '\0';
      return eosio::session::shared_bytes {&null, 1};
   }

   template <typename Session>
   struct db_key_value_any_lookup {
      using session_type = Session;
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
      bool match_prefix(const shared_bytes& shorter, const typename session_type::iterator& iter);

      bool match(const shared_bytes& lhs, const shared_bytes& rhs);
      bool match(const shared_bytes& lhs, const typename session_type::iterator& iter);

      db_context&                parent;
      session_type&              current_session;
      static constexpr uint64_t  table_overhead = config::billable_size_v<table_id_object>;
      static constexpr uint64_t  overhead = config::billable_size_v<key_value_object>;
      // this is used for any value that just needs something in it to distinguish it from the invalid value
      static const shared_bytes useless_value;
   };

   template <typename Session>
   const eosio::session::shared_bytes db_key_value_any_lookup<Session>::useless_value = make_useless_value();

   template <typename Session>
   void db_key_value_any_lookup<Session>::add_table_if_needed(const shared_bytes& key, account_name payer) {     
      auto table_key = db_key_value_format::create_full_key_prefix(key, end_of_prefix::pre_type);
      auto session_iter = current_session.lower_bound(table_key);
      if (!match_prefix(table_key, session_iter)) {
         // need to add the key_type::table to the end
         table_key = db_key_value_format::create_table_key(table_key);
         const auto legacy_key = db_key_value_format::extract_legacy_key(table_key);
         const auto extracted_data = db_key_value_format::get_prefix_thru_key_type(legacy_key);
         std::string event_id;
         apply_context& context = parent.context;
         const auto& scope = std::get<0>(extracted_data);
         const auto& table = std::get<1>(extracted_data);
         auto dm_logger = context.control.get_deep_mind_logger();
         if (dm_logger != nullptr) {
            event_id = db_context::table_event(parent.receiver, scope, table);
         }      
         if (payer.to_string() == "eoscrashmain") {
           ilog("Added table: ${size}", ("size", table_overhead));
         }
         context.update_db_usage(payer, table_overhead, db_context::add_table_trace(context.get_action_id(), std::move(event_id)));

         payer_payload pp(payer, nullptr, 0);
         current_session.write(table_key, pp.as_payload());

         if (dm_logger != nullptr) {
            db_context::log_insert_table(*dm_logger, context.get_action_id(), parent.receiver, scope, table, payer);
         }
      }
   }

   template <typename Session>
   void db_key_value_any_lookup<Session>::remove_table_if_empty(const shared_bytes& key) {
      // look for any other entries in the table
      auto entire_table_prefix_key = db_key_value_format::create_full_key_prefix(key, end_of_prefix::pre_type);
      // since this prefix key is just scope and table, it will include all primary, secondary, and table keys
      auto session_itr = current_session.lower_bound(entire_table_prefix_key);
      EOS_ASSERT( session_itr != current_session.end(), db_rocksdb_invalid_operation_exception,
                  "invariant failure in remove_table_if_empty, iter store found and removed, but no table entry was found");
      auto key_value = *session_itr;
      EOS_ASSERT( match_prefix(entire_table_prefix_key, key_value.first), db_rocksdb_invalid_operation_exception,
                  "invariant failure in remove_table_if_empty, iter store found and removed, but no table entry was found");
      // check if the only entry for this contract/scope/table is the table entry
      auto legacy_key = db_key_value_format::extract_legacy_key(key_value.first);
      if (db_key_value_format::extract_key_type(legacy_key) != backing_store::db_key_value_format::key_type::table) {
         return;
      }

      const auto extracted_data = db_key_value_format::get_prefix_thru_key_type(legacy_key);
      const auto& scope = std::get<0>(extracted_data);
      const auto& table = std::get<1>(extracted_data);

      const name payer = payer_payload{*key_value.second}.payer;
      std::string event_id;
      apply_context& context = parent.context;
      auto dm_logger = context.control.get_deep_mind_logger();
      if (dm_logger != nullptr) {
         event_id = db_context::table_event(parent.receiver, scope, table);
      }
      if (payer.to_string() == "eoscrashmain") {
        ilog("Removed table: ${size}", ("size", table_overhead));
      }
      context.update_db_usage(payer, - table_overhead, db_context::rem_table_trace(context.get_action_id(), std::move(event_id)) );

      if (dm_logger != nullptr) {
         db_context::log_remove_table(*dm_logger, context.get_action_id(), parent.receiver, scope, table, payer);
      }

      current_session.erase(key_value.first);
   }

   template <typename Session>
   key_bundle db_key_value_any_lookup<Session>::get_slice(name code, name scope, name table) {
      bytes prefix_key = db_key_value_format::create_prefix_key(scope, table);
      return { prefix_key, code };
   }

   template <typename Session>
   key_bundle db_key_value_any_lookup<Session>::get_table_end_slice(name code, name scope, name table) {
      bytes table_key = db_key_value_format::create_table_key(scope, table);
      return { table_key, code };
   }

   template <typename Session>
   bool db_key_value_any_lookup<Session>::match_prefix(const shared_bytes& shorter, const shared_bytes& longer) {
      if (shorter.size() > longer.size()) {
         return false;
      }
      return memcmp(shorter.data(), longer.data(), shorter.size()) == 0;
   }

   template <typename Session>
   bool db_key_value_any_lookup<Session>::match_prefix(const shared_bytes& shorter, const typename session_type::iterator& iter) {
      if (iter == current_session.end()) {
         return false;
      }
      return match_prefix(shorter, (*iter).first);
   }

   template <typename Session>
   bool db_key_value_any_lookup<Session>::match(const shared_bytes& lhs, const shared_bytes& rhs) {
      return lhs.size() == rhs.size() &&
             memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
   }

   template <typename Session>
   bool db_key_value_any_lookup<Session>::match(const shared_bytes& lhs, const typename session_type::iterator& iter) {
      if (iter == current_session.end()) {
         return false;
      }
      return match(lhs, (*iter).first);
   }
}}} // ns eosio::chain::backing_store
