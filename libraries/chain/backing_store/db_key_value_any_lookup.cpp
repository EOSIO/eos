#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_key_value_any_lookup.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>

namespace eosio { namespace chain { namespace backing_store {

   void db_key_value_any_lookup::add_table_if_needed(name scope, name table, account_name payer) {
      const bytes table_key = db_key_value_format::create_table_key(scope, table);
      const rocksdb::Slice slice_table_key{table_key.data(), table_key.size()};
      if (!view.get(parent.receiver.to_uint64_t(), slice_table_key)) {
         std::string event_id;
         apply_context& context = parent.context;
         if (context.control.get_deep_mind_logger() != nullptr) {
            event_id = db_context::table_event(parent.receiver, scope, table);
         }

         context.update_db_usage(payer, table_overhead, db_context::add_table_trace(context.get_action_id(), event_id.c_str()));

         char payer_buf[payer_in_value_size];
         memcpy(payer_buf, &payer, payer_in_value_size);
         view.set(parent.receiver.to_uint64_t(), slice_table_key, {payer_buf, payer_in_value_size});

         if (auto dm_logger = context.control.get_deep_mind_logger()) {
            db_context::write_insert_table(*dm_logger, context.get_action_id(), parent.receiver, scope, table, payer);
         }
      }
   }

   void db_key_value_any_lookup::remove_table_if_empty(name scope, name table) {
      // look for any other entries in the table
      const key_bundle slice_prefix_key = get_slice(scope, table);
      // since this prefix key is just scope and table, it will include all primary, secondary, and table keys
      b1::chain_kv::view::iterator chain_kv_iter{ view, parent.receiver.to_uint64_t(), slice_prefix_key.key };
      chain_kv_iter.move_to_begin();
      const auto first_kv = chain_kv_iter.get_kv();
      EOS_ASSERT( first_kv, db_rocksdb_invalid_operation_exception,
                  "invariant failure in db_remove_i64, iter store found and removed, but no table entry was found");
      name returned_scope;
      name returned_table;
      b1::chain_kv::bytes key(first_kv->key.data(), first_kv->key.data() + first_kv->key.size());
      // check if the only entry for this contract/scope/table is the table entry
      if (!backing_store::db_key_value_format::get_table_key(key, returned_scope, returned_table)) {
         return;
      }

      EOS_ASSERT( returned_scope == scope && returned_table == table, db_rocksdb_invalid_operation_exception,
                  "invariant failure in db_remove_i64, iter store found and removed, but chain_kv returned key "
                  "outside relevant range");

      const name payer = payer_payload{first_kv->value.data(), first_kv->value.size()}.payer;
      std::string event_id;
      apply_context& context = parent.context;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = db_context::table_event(parent.receiver, scope, table);
      }

      context.update_db_usage(payer, - table_overhead, db_context::rem_table_trace(context.get_action_id(), event_id.c_str()) );

      if (auto dm_logger = context.control.get_deep_mind_logger()) {
         db_context::write_remove_table(*dm_logger, context.get_action_id(), parent.receiver, scope, table, payer);
      }

      view.erase(parent.receiver.to_uint64_t(), first_kv->key);
   }

   key_bundle db_key_value_any_lookup::get_slice(name scope, name table) {
      bytes prefix_key = db_key_value_format::create_prefix_key(scope, table);
      rocksdb::Slice key {prefix_key.data(), prefix_key.size()};
      return { .composite_key = std::move(prefix_key), .key = std::move(key) };
   }

   key_bundle db_key_value_any_lookup::get_table_end_slice(name scope, name table) {
      bytes table_key = db_key_value_format::create_table_key(scope, table);
      rocksdb::Slice key = {table_key.data(), table_key.size()};
      return { .composite_key = std::move(table_key), .key = std::move(key) };
   }


}}} // namespace eosio::chain::backing_store
