#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_key_value_any_lookup.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>
#include <eosio/chain/combined_database.hpp>

namespace eosio { namespace chain { namespace backing_store {

   eosio::session::shared_bytes make_useless_value() {
      const char null = '\0';
      return eosio::session::shared_bytes {&null, 1};
   }

   const eosio::session::shared_bytes db_key_value_any_lookup::useless_value = make_useless_value();

   void db_key_value_any_lookup::add_table_if_needed(const shared_bytes& key, account_name payer) {
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

         context.update_db_usage(payer, table_overhead, db_context::add_table_trace(context.get_action_id(), event_id));

         payer_payload pp(payer, nullptr, 0);
         current_session.write(table_key, pp.as_payload());

         if (dm_logger != nullptr) {
            db_context::write_insert_table(*dm_logger, context.get_action_id(), parent.receiver, scope, table, payer);
         }
      }
   }

   void db_key_value_any_lookup::remove_table_if_empty(const shared_bytes& key) {
      // look for any other entries in the table
      auto entire_table_prefix_key = db_key_value_format::create_full_key_prefix(key, end_of_prefix::at_type);
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

      context.update_db_usage(payer, - table_overhead, db_context::rem_table_trace(context.get_action_id(), event_id) );

      if (dm_logger != nullptr) {
         db_context::write_remove_table(*dm_logger, context.get_action_id(), parent.receiver, scope, table, payer);
      }

      current_session.erase(key_value.first);
   }

   key_bundle db_key_value_any_lookup::get_slice(name code, name scope, name table) {
      bytes prefix_key = db_key_value_format::create_prefix_key(scope, table);
      return { prefix_key, code };
   }

   key_bundle db_key_value_any_lookup::get_table_end_slice(name code, name scope, name table) {
      bytes table_key = db_key_value_format::create_table_key(scope, table);
      return { table_key, code };
   }

   bool db_key_value_any_lookup::match_prefix(const shared_bytes& shorter, const shared_bytes& longer) {
      if (shorter.size() > longer.size()) {
         return false;
      }
      return memcmp(shorter.data(), longer.data(), shorter.size()) == 0;
   }

   bool db_key_value_any_lookup::match_prefix(const shared_bytes& shorter, const session_type::iterator& iter) {
      if (iter == current_session.end()) {
         return false;
      }
      return match_prefix(shorter, (*iter).first);
   }

   bool db_key_value_any_lookup::match(const shared_bytes& lhs, const shared_bytes& rhs) {
      return lhs.size() == rhs.size() &&
             memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
   }

   bool db_key_value_any_lookup::match(const shared_bytes& lhs, const session_type::iterator& iter) {
      if (iter == current_session.end()) {
         return false;
      }
      return match(lhs, (*iter).first);
   }

   key_bundle::key_bundle(const b1::chain_kv::bytes& composite_key, name code)
   : full_key(db_key_value_format::create_full_key(composite_key, code)){
   }

   prefix_bundle::prefix_bundle(const b1::chain_kv::bytes& composite_key, end_of_prefix prefix_end, name code)
   : full_key(db_key_value_format::create_full_key(composite_key, code)),
     prefix_key(db_key_value_format::create_full_key_prefix(full_key, prefix_end)) {
   }

}}} // namespace eosio::chain::backing_store
