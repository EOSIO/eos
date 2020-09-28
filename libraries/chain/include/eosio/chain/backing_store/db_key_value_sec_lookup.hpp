#pragma once

#include <eosio/chain/backing_store/chain_kv_payer.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace backing_store {

   template<typename SecondaryKey>
   struct secondary_helper;

   template<>
   struct secondary_helper<uint64_t> {
      const char* desc() { return "idx64"; }
      bytes value(const uint64_t& sec_key, name payer) {
         return payer_payload(payer, nullptr, 0).as_payload();
      }
      void extract(const bytes& payload, uint64_t& sec_key, name& payer) {
         payer_payload pp(payload);
         payer = pp.payer;
         EOS_ASSERT( pp.value_size == 0, db_rocksdb_invalid_operation_exception,
                     "Payload should not have anything more than the payer");
      }
      uint32_t overhead() { return config::billable_size_v<index64_object>;}
   };

   template<>
   struct secondary_helper<eosio::chain::uint128_t> {
      const char* desc() { return "idx128"; }
      bytes value(const eosio::chain::uint128_t& sec_key, name payer) {
         return payer_payload(payer, nullptr, 0).as_payload();
      }
      void extract(const bytes& payload, eosio::chain::uint128_t& sec_key, name& payer) {
         payer_payload pp(payload);
         payer = pp.payer;
         EOS_ASSERT( pp.value_size == 0, db_rocksdb_invalid_operation_exception,
                     "Payload should not have anything more than the payer");
      }
      uint32_t overhead() { return config::billable_size_v<index128_object>;}
   };

   template<>
   struct secondary_helper<eosio::chain::key256_t> {
      const char* desc() { return "idx256"; }
      bytes value(const eosio::chain::key256_t& sec_key, name payer) {
         return payer_payload(payer, nullptr, 0).as_payload();
      }
      void extract(const bytes& payload, eosio::chain::key256_t& sec_key, name& payer) {
         payer_payload pp(payload);
         payer = pp.payer;
         EOS_ASSERT( pp.value_size == 0, db_rocksdb_invalid_operation_exception,
                     "Payload should not have anything more than the payer");
      }
      uint32_t overhead() { return config::billable_size_v<index256_object>;}
   };

   template<>
   struct secondary_helper<float64_t> {
      const char* desc() { return "idx_double"; }
      bytes value(const float64_t& sec_key, name payer) {
         constexpr static auto value_size = sizeof(sec_key);
         char value_buf[value_size];
         memcpy(value_buf, &sec_key, value_size);
         return payer_payload(payer, value_buf, value_size).as_payload();
      }
      void extract(const bytes& payload, float64_t& sec_key, name& payer) {
         payer_payload pp(payload);
         EOS_ASSERT( pp.value_size == sizeof(sec_key), db_rocksdb_invalid_operation_exception,
                     "Payload does not contain the expected float64_t exact representation");
         payer = pp.payer;
         memcpy(&sec_key, pp.value, pp.value_size);
      }
      uint32_t overhead() { return config::billable_size_v<index_double_object>;}
   };

   template<>
   struct secondary_helper<float128_t> {
      const char* desc() { return "idx_long_double"; }
      bytes value(const float128_t& sec_key, name payer) {
         constexpr static auto value_size = sizeof(sec_key);
         char value_buf[value_size];
         memcpy(value_buf, &sec_key, value_size);
         return payer_payload(payer, value_buf, value_size).as_payload();
      }
      void extract(const bytes& payload, float128_t& sec_key, name& payer) {
         payer_payload pp(payload);
         EOS_ASSERT( pp.value_size == sizeof(sec_key), db_rocksdb_invalid_operation_exception,
                     "Payload does not contain the expected float128_t exact representation");
         payer = pp.payer;
         memcpy(&sec_key, pp.value, pp.value_size);
      }
      uint32_t overhead() { return config::billable_size_v<index_long_double_object>;}
   };

   template<typename SecondaryKey >
   class db_key_value_sec_lookup : public db_key_value_any_lookup
   {
   public:
      db_key_value_sec_lookup( db_context& p, b1::chain_kv::view& v ) : db_key_value_any_lookup(p, v) {}

      int store( name scope, name table, const account_name& payer,
                 uint64_t id, const SecondaryKey& secondary ) {
         EOS_ASSERT( payer != account_name(), invalid_table_payer, "must specify a valid account to pay for new record" );

         add_table_if_needed(scope, table, payer);

         const sec_pair_bundle secondary_key = get_secondary_slices_in_secondaries(scope, table, secondary, id);
         std::shared_ptr<const bytes> old_value = view.get(parent.receiver.to_uint64_t(), secondary_key.key);

         EOS_ASSERT( !old_value, db_rocksdb_invalid_operation_exception, "db_${d}_store called with pre-existing key", ("d", helper.desc()));

         // identify if this primary key already has a secondary key of this type
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(parent.receiver, secondary_key.primary_to_sec_type_prefix,
                                                                      secondary_key.primary_to_sec_type_prefix);

         const auto found_kv = chain_kv_iter.get_kv();
         EOS_ASSERT( !found_kv, db_rocksdb_invalid_operation_exception,
                     "db_${d}_store called, but primary key: ${primary} already has a secondary key of this type",
                     ("d", helper.desc())("primary", id));

         set_value(secondary_key.key, helper.value(secondary, payer));
         const b1::chain_kv::bytes empty;
         set_value(secondary_key.primary_to_sec_key, empty);

         std::string event_id;
         if (parent.context.control.get_deep_mind_logger() != nullptr) {
            event_id = db_context::table_event(parent.receiver, scope, table, name(id));
         }

         parent.context.update_db_usage( payer, helper.overhead(), backing_store::db_context::secondary_add_trace(parent.context.get_action_id(), event_id.c_str()) );

         const unique_table t { parent.receiver, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         return iter_store.add({ .table_ei = table_ei, .secondary = secondary, .primary = id, .payer = payer });
      }

      void remove( int iterator ) {
         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);
         EOS_ASSERT( table.contract == parent.receiver, table_access_violation, "db access violation" );

         const sec_pair_bundle secondary_key = get_secondary_slices_in_secondaries(table.scope, table.table, key_store.secondary, key_store.primary);
         std::shared_ptr<const bytes> old_value = view.get(parent.receiver.to_uint64_t(), secondary_key.key);

         EOS_ASSERT( old_value, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_remove, iter store found to update but nothing in database", ("d", helper.desc()));

         // identify if this primary key already has a secondary key of this type
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(parent.receiver, secondary_key.primary_to_sec_type_prefix,
                                                                      secondary_key.primary_to_sec_type_prefix);
         const auto found_kv = chain_kv_iter.get_kv();
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_remove, the secondary to primary lookup key was found, but not the "
                     "primary to secondary lookup key", ("d", helper.desc()));

         std::string event_id;
         if (parent.context.control.get_deep_mind_logger() != nullptr) {
            event_id = db_context::table_event(table.contract, table.scope, table.table, name(key_store.primary));
         }

         parent.context.update_db_usage( key_store.payer, -( helper.overhead() ), db_context::secondary_rem_trace(parent.context.get_action_id(), event_id.c_str()) );

         view.erase(parent.receiver.to_uint64_t(), secondary_key.key);
         view.erase(parent.receiver.to_uint64_t(), secondary_key.primary_to_sec_key);
         remove_table_if_empty(table.scope, table.table);
         iter_store.remove(iterator);
      }

      void update( int iterator, account_name payer, const SecondaryKey& secondary ) {
         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);
         EOS_ASSERT( table.contract == parent.receiver, table_access_violation, "db access violation" );

         const sec_pair_bundle secondary_key = get_secondary_slices_in_secondaries(table.scope, table.table, key_store.secondary, key_store.primary);
         std::shared_ptr<const bytes> old_value = view.get(parent.receiver.to_uint64_t(), secondary_key.key);

         secondary_helper<SecondaryKey> helper;
         EOS_ASSERT( old_value, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_remove, iter store found to update but nothing in database", ("d", helper.desc()));

         // identify if this primary key already has a secondary key of this type
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(parent.receiver, secondary_key.primary_to_sec_type_prefix,
                                                                      secondary_key.primary_to_sec_type_prefix);
         const auto found_kv = chain_kv_iter.get_kv();
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_update, the secondary to primary lookup key was found, but not the "
                     "primary to secondary lookup key", ("d", helper.desc()));
         EOS_ASSERT( match(found_kv->key, secondary_key.primary_to_sec_key), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_update, database returned a primary to secondary lookup key with "
                     "${info}",
                     ("d", helper.desc())
                     ("info", (found_kv->key.size() != secondary_key.primary_to_sec_key.size() ?
                               "different sized strings" : "a different secondary key of the same type")));

         if( payer == account_name() ) payer = key_store.payer;

         auto& context = parent.context;
         std::string event_id;
         if (context.control.get_deep_mind_logger() != nullptr) {
            event_id = backing_store::db_context::table_event(table.contract, table.scope, table.table, name(key_store.primary));
         }

         if( key_store.payer != payer ) {
            context.update_db_usage( key_store.payer, -(helper.overhead()), backing_store::db_context::secondary_update_rem_trace(context.get_action_id(), event_id.c_str()) );
            context.update_db_usage( payer, +(helper.overhead()), backing_store::db_context::secondary_update_add_trace(context.get_action_id(), event_id.c_str()) );
         }

         // if the secondary value is different, remove the old key and add the new key
         if (secondary != key_store.secondary) {
            // remove the secondary (to primary) key and the primary to secondary key
            view.erase(parent.receiver.to_uint64_t(), secondary_key.key);
            view.erase(parent.receiver.to_uint64_t(), secondary_key.primary_to_sec_key);

            // store the new secondary (to primary) key
            const auto new_secondary_keys = db_key_value_format::create_secondary_key_pair(table.scope, table.table, secondary, key_store.primary);
            const rocksdb::Slice new_sec_key = {new_secondary_keys.secondary_key.data(), new_secondary_keys.secondary_key.size()};
            set_value(new_sec_key, helper.value(secondary, payer));

            // store the new primary to secondary key
            const b1::chain_kv::bytes empty;
            const rocksdb::Slice new_prim_to_sec_key = {new_secondary_keys.primary_to_secondary_key.data(),
                                                        new_secondary_keys.primary_to_secondary_key.size()};
            set_value(new_prim_to_sec_key, empty);
         }
         else {
            // no key values have changed, so can use the old key (and no change to secondary to primary key)
            set_value(secondary_key.key, helper.value(secondary, payer));
         }
         iter_store.swap(iterator, secondary, payer);
      }

      int find_secondary( name code, name scope, name table, const SecondaryKey& secondary, uint64_t& primary ) {
         prefix_bundle secondary_key = get_secondary_slice_in_table(scope, table, secondary);
         // setting the "key space" to be the whole table, so that we either get a match or another key for this table
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(code, secondary_key.prefix, secondary_key.key);

         const auto found_kv = chain_kv_iter.get_kv();
         if (!found_kv) {
            // no keys for this entire table
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         // verify if the key that was found contains this secondary key
         if (!match_prefix(secondary_key.key, found_kv->key)) {
            return table_ei;
         }

         const bool valid_key = db_key_value_format::get_trailing_primary_key(found_kv->key, secondary_key.key, primary);
         EOS_ASSERT( valid_key, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_find_secondary, key returned by rocksdb should have matched",
                     ("d", helper.desc()));

         return iter_store.add({ .table_ei = table_ei, .secondary = secondary, .primary = primary,
                                 .payer = payer_payload(found_kv->value.data(), found_kv->value.size()).payer });
      }

      int lowerbound_secondary( name code, name scope, name table, SecondaryKey& secondary, uint64_t& primary ) {
         return bound_secondary(code, scope, table, bound_type::lower, secondary, primary);
      }

      int upperbound_secondary( name code, name scope, name table, SecondaryKey& secondary, uint64_t& primary ) {
         return bound_secondary(code, scope, table, bound_type::upper, secondary, primary);
      }

      int end_secondary( name code, name scope, name table ) {
         return get_end_iter(name{code}, name{scope}, name{table}, iter_store);
      }

      int next_secondary( int iterator, uint64_t& primary ) {
         if( iterator < iter_store.invalid_iterator() ) return iter_store.invalid_iterator(); // cannot increment past end iterator of index

         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);

         prefix_bundle secondary_key = get_secondary_slice_in_secondaries(table.scope, table.table, key_store.secondary, key_store.primary);
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(table.contract, secondary_key.prefix, secondary_key.key);

         auto found_kv = chain_kv_iter.get_kv();
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, found iterator in iter store but didn't find "
                     "any entry in the database (no secondary keys of this specific type)", ("d", helper.desc()));
         EOS_ASSERT( match(found_kv->key, secondary_key.key), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, found iterator in iter store but database returned a "
                     "totally different secondary key of the same type", ("d", helper.desc()));
         ++chain_kv_iter;
         found_kv = chain_kv_iter.get_kv();
         if (!found_kv) {
            return key_store.table_ei;
         }

         SecondaryKey secondary;
         const bool valid_key = db_key_value_format::get_trailing_sec_prim_keys(found_kv->key, secondary_key.prefix, secondary, primary);
         EOS_ASSERT( valid_key, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, since the type slice matched, the trailing "
                     "primary key should have been extracted", ("d", helper.desc()));

         return iter_store.add({ .table_ei = key_store.table_ei, .secondary = secondary, .primary = primary,
                                 .payer = payer_payload(found_kv->value.data(), found_kv->value.size()).payer });
      }

      int previous_secondary( int iterator, uint64_t& primary ) {
         if( iterator < iter_store.invalid_iterator() ) {
            const backing_store::unique_table* table = iter_store.find_table_by_end_iterator(iterator);
            EOS_ASSERT( table, invalid_table_iterator, "not a valid end iterator" );
            constexpr static auto kt = db_key_value_format::derive_secondary_key_type<SecondaryKey>();
            const bytes type_key = db_key_value_format::create_prefix_type_key<kt>(table->scope, table->table);
            const rocksdb::Slice key = {type_key.data(), type_key.size()};
            b1::chain_kv::view::iterator chain_kv_iter { view, table->contract.to_uint64_t(), key };
            chain_kv_iter.move_to_end();
            --chain_kv_iter; // move back to the last secondary key of this type (if there is one)
            const auto found_kv = chain_kv_iter.get_kv();
            // check if nothing remains in the database
            if (!found_kv) {
               return iter_store.invalid_iterator();
            }
            auto found_keys = find_matching_sec_prim(*found_kv, type_key);
            if (!found_keys) {
               return iter_store.invalid_iterator();
            }
            return iter_store.add({ .table_ei = iterator, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                    .primary = std::move(std::get<fk_index::primary>(*found_keys)),
                                    .payer = std::move(std::get<fk_index::payer>(*found_keys))});
         }

         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);

         prefix_bundle secondary_key = get_secondary_slice_in_secondaries(table.scope, table.table, key_store.secondary, key_store.primary);
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(table.contract, secondary_key.prefix, secondary_key.key);

         auto found_kv = chain_kv_iter.get_kv();
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, found iterator in iter store but didn't find "
                     "any entry in the database (no secondary keys of this specific type)", ("d", helper.desc()));
         EOS_ASSERT( match(found_kv->key, secondary_key.key), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, found iterator in iter store but didn't find its "
                     "matching entry in the database", ("d", helper.desc()));
         --chain_kv_iter;
         found_kv = chain_kv_iter.get_kv();
         if (!found_kv) {
            return key_store.table_ei;
         }

         SecondaryKey secondary;
         const bool valid_key = db_key_value_format::get_trailing_sec_prim_keys(found_kv->key, secondary_key.prefix, secondary, primary);
         EOS_ASSERT( valid_key, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_secondary, since the type slice matched, the trailing "
                     "primary key should have been extracted", ("d", helper.desc()));

         return iter_store.add({ .table_ei = key_store.table_ei, .secondary = secondary, .primary = primary,
                                 .payer = payer_payload(found_kv->value.data(), found_kv->value.size()).payer });
      }

      int find_primary( name code, name scope, name table, SecondaryKey& secondary, uint64_t primary ) {
         const bytes prim_to_sec_key = db_key_value_format::create_prefix_primary_to_secondary_key<SecondaryKey>(scope, table, primary);
         const rocksdb::Slice key = {prim_to_sec_key.data(), prim_to_sec_key.size()};
         const rocksdb::Slice type_prefix = db_key_value_format::prefix_type_slice(prim_to_sec_key);
         const rocksdb::Slice prefix = db_key_value_format::prefix_slice(prim_to_sec_key);
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(code, prefix, key);

         const auto found_kv = chain_kv_iter.get_kv();
         // check if nothing remains in the table database
         if (!found_kv) {
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);

         // if this is not of the same type and primary key
         if (!backing_store::db_key_value_format::get_trailing_secondary_key(found_kv->key, key, secondary)) {
            // since this is not the desired secondary key entry, reject it
            return table_ei;
         }

         const bytes secondary_key = db_key_value_format::create_secondary_key(scope, table, secondary, primary);
         std::shared_ptr<const bytes> old_value = view.get(code.to_uint64_t(), {secondary_key.data(), secondary_key.size()});
         EOS_ASSERT( old_value, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_secondary, found primary to secondary key in database but "
                     "not the secondary to primary key", ("d", helper.desc()));
         EOS_ASSERT( old_value->data() != nullptr, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_secondary, empty value", ("d", helper.desc()));
         EOS_ASSERT( old_value->size(), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_secondary, value has zero size", ("d", helper.desc()));
         payer_payload pp{*old_value};

         return iter_store.add({ .table_ei = table_ei, .secondary = secondary, .primary = primary,
                                 .payer = pp.payer});
      }

      int lowerbound_primary( name code, name scope, name table, uint64_t primary ) {
         return bound_primary(code, scope, table, primary, bound_type::lower);
      }

      int upperbound_primary( name code, name scope, name table, uint64_t primary ) {
         return bound_primary(code, scope, table, primary, bound_type::upper);
      }

      int next_primary( int iterator, uint64_t& primary ) {
         if( iterator < iter_store.invalid_iterator() ) return iter_store.invalid_iterator(); // cannot increment past end iterator of index

         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);

         const bytes prim_to_sec_key =
               db_key_value_format::create_primary_to_secondary_key<SecondaryKey>(table.scope, table.table, key_store.primary, key_store.secondary);
         const rocksdb::Slice key = {prim_to_sec_key.data(), prim_to_sec_key.size()};
         const rocksdb::Slice prefix =
               db_key_value_format::prefix_primary_to_secondary_slice(prim_to_sec_key, false);
         // search for exact key, but in the range of all primary to secondaries of this type
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(table.contract, prefix, key);
         auto found_kv = chain_kv_iter.get_kv();
         // check if nothing remains in the table database
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_primary, found iterator in iter store but didn't find its "
                     "matching entry in the database", ("d", helper.desc()));
         ++chain_kv_iter;
         found_kv = chain_kv_iter.get_kv();
         if (!found_kv) {
            return key_store.table_ei;
         }

         auto found_keys = find_matching_prim_sec(*found_kv, prefix);
         if (!found_keys) {
            return key_store.table_ei;
         }
         primary = std::get<fk_index::primary>(*found_keys);
         return iter_store.add({ .table_ei = key_store.table_ei, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                 .primary = std::move(std::get<fk_index::primary>(*found_keys)),
                                 .payer = std::move(std::get<fk_index::payer>(*found_keys))});
      }

      int previous_primary( int iterator, uint64_t& primary ) {
         if( iterator < iter_store.invalid_iterator() ) {
            const backing_store::unique_table* table = iter_store.find_table_by_end_iterator(iterator);
            EOS_ASSERT( table, invalid_table_iterator, "not a valid end iterator" );
            const bytes types_key = db_key_value_format::create_prefix_primary_to_secondary_key<SecondaryKey>(table->scope, table->table);
            const rocksdb::Slice prefix = {types_key.data(), types_key.size()};
            b1::chain_kv::view::iterator chain_kv_iter { view, table->contract.to_uint64_t(), prefix };
            chain_kv_iter.move_to_end();
            --chain_kv_iter; // move back to the last primary key of this type (if there is one)
            const auto found_kv = chain_kv_iter.get_kv();
            // check if nothing remains in the database
            if (!found_kv) {
               return iter_store.invalid_iterator();
            }
            auto found_keys = find_matching_prim_sec(*found_kv, types_key);
            if (!found_keys) {
               return iter_store.invalid_iterator();
            }
            primary = std::move(std::get<fk_index::primary>(*found_keys));
            return iter_store.add({ .table_ei = iterator, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                    .primary = std::move(std::get<fk_index::primary>(*found_keys)),
                                    .payer = std::move(std::get<fk_index::payer>(*found_keys))});
         }

         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);

         const bytes prim_to_sec_key =
               db_key_value_format::create_primary_to_secondary_key<SecondaryKey>(table.scope, table.table, key_store.primary, key_store.secondary);
         const rocksdb::Slice key = {prim_to_sec_key.data(), prim_to_sec_key.size()};
         const rocksdb::Slice prefix =
               db_key_value_format::prefix_primary_to_secondary_slice(prim_to_sec_key, false);
         // search for exact key, but in the range of all primary to secondaries of this type
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(table.contract, prefix, key);
         auto found_kv = chain_kv_iter.get_kv();
         // check if nothing remains in the table database
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_primary, found iterator in iter store but didn't find its "
                     "matching entry in the database", ("d", helper.desc()));
         --chain_kv_iter;
         found_kv = chain_kv_iter.get_kv();
         if (!found_kv) {
            return iter_store.invalid_iterator();
         }

         auto found_keys = find_matching_prim_sec(*found_kv, prefix);
         if (!found_keys) {
            return iter_store.invalid_iterator();
         }
         primary = std::get<fk_index::primary>(*found_keys);
         return iter_store.add({ .table_ei = key_store.table_ei, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                 .primary = std::move(std::get<fk_index::primary>(*found_keys)),
                                 .payer = std::move(std::get<fk_index::payer>(*found_keys))});
      }

      void get( int iterator, uint64_t& primary, SecondaryKey& secondary ) {
         const iter_obj& key_store = iter_store.get(iterator);

         primary = key_store.primary;
         secondary = key_store.secondary;
      }

      struct sec_pair_bundle {
         bytes          composite_key;
         bytes          primary_to_sec_comp_key;
         rocksdb::Slice key;                         // points to all of composite_key
         rocksdb::Slice sec_prefix;                  // points to front type prefix for composite_key
         rocksdb::Slice primary_to_sec_key;          // points to all of primary_to_sec_comp_key
         rocksdb::Slice primary_to_sec_type_prefix;  // points to front type prefix for secondary in primary_to_sec_comp_key (all except the secondary key)
      };

      void REMOVE(const sec_pair_bundle& bundle) {
         EOS_ASSERT( bundle.composite_key.size(), db_rocksdb_invalid_operation_exception,
                     "bundle composite_key is empty");
         EOS_ASSERT( bundle.key.data() == bundle.composite_key.data(), db_rocksdb_invalid_operation_exception,
                     "bundle key doesn't match");
         EOS_ASSERT( bundle.sec_prefix.data() == bundle.composite_key.data(), db_rocksdb_invalid_operation_exception,
                     "bundle prefix key doesn't match");
         EOS_ASSERT( bundle.primary_to_sec_comp_key.size(), db_rocksdb_invalid_operation_exception,
                     "bundle primary to sec key is empty");
         EOS_ASSERT( bundle.primary_to_sec_key.data() == bundle.primary_to_sec_comp_key.data(), db_rocksdb_invalid_operation_exception,
                     "bundle primary to sec key doesn't match");
         EOS_ASSERT( bundle.primary_to_sec_type_prefix.data() == bundle.primary_to_sec_comp_key.data(), db_rocksdb_invalid_operation_exception,
                     "bundle primary to sec key prefix doesn't match");
      }

      // gets a prefix that allows for only this secondary key iterators, and the primary to secondary key, with a prefix key to the secondary key type
      static sec_pair_bundle get_secondary_slices_in_secondaries(name scope, name table, const SecondaryKey& secondary, uint64_t primary) {
         auto secondary_keys = db_key_value_format::create_secondary_key_pair(scope, table, secondary, primary);
         rocksdb::Slice key = {secondary_keys.secondary_key.data(), secondary_keys.secondary_key.size()};
         rocksdb::Slice prefix = db_key_value_format::prefix_type_slice(secondary_keys.secondary_key);
         rocksdb::Slice primary_to_sec_key = {secondary_keys.primary_to_secondary_key.data(), secondary_keys.primary_to_secondary_key.size()};
         rocksdb::Slice primary_to_sec_type_prefix =
               db_key_value_format::prefix_primary_to_secondary_slice(secondary_keys.primary_to_secondary_key, true);
         return { .composite_key = std::move(secondary_keys.secondary_key),
                  .primary_to_sec_comp_key = std::move(secondary_keys.primary_to_secondary_key),
                  .key = std::move(key),
                  .sec_prefix = std::move(prefix),
                  .primary_to_sec_key = std::move(primary_to_sec_key),
                  .primary_to_sec_type_prefix = std::move(primary_to_sec_type_prefix) };
      }

      // gets a specific secondary key without the trailing primary key, and will allow for only secondaries of this type (prefix)
      static prefix_bundle get_secondary_slice_in_secondaries(name scope, name table, const SecondaryKey& secondary) {
         bytes secondary_key = db_key_value_format::create_prefix_secondary_key(scope, table, secondary);
         rocksdb::Slice prefix = db_key_value_format::prefix_type_slice(secondary_key);
         rocksdb::Slice key = {secondary_key.data(), secondary_key.size()};
         return { .composite_key = std::move(secondary_key), .key = std::move(key), .prefix = std::move(prefix) };
      }

      // gets a specific secondary and primary keys, and will allow for only secondaries of this type (prefix)
      static prefix_bundle get_secondary_slice_in_secondaries(name scope, name table, const SecondaryKey& secondary, uint64_t primary) {
         bytes secondary_key = db_key_value_format::create_secondary_key(scope, table, secondary, primary);
         rocksdb::Slice prefix = db_key_value_format::prefix_type_slice(secondary_key);
         rocksdb::Slice key = {secondary_key.data(), secondary_key.size()};
         return { .composite_key = std::move(secondary_key), .key = std::move(key), .prefix = std::move(prefix) };
      }

      // gets a prefix that allows for a specific secondary key without the trailing primary key, but will allow for all iterators in the table
      static prefix_bundle get_secondary_slice_in_table(name scope, name table, const SecondaryKey& secondary) {
         bytes secondary_key = db_key_value_format::create_prefix_secondary_key(scope, table, secondary);
         rocksdb::Slice prefix = db_key_value_format::prefix_slice(secondary_key);
         rocksdb::Slice key = {secondary_key.data(), secondary_key.size()};
         return { .composite_key = std::move(secondary_key), .key = std::move(key), .prefix = std::move(prefix) };
      }

      void set_value(const rocksdb::Slice& secondary_composite_key, const bytes& payload) {
         rocksdb::Slice payload_slice {payload.data(), payload.size()};
         view.set(parent.receiver.to_uint64_t(), secondary_composite_key, payload_slice);
      }

   private:
      bool match_prefix(const rocksdb::Slice& shorter, const rocksdb::Slice& longer) {
         EOS_ASSERT( shorter.size() <= longer.size(), db_rocksdb_invalid_operation_exception,
                     "invariant failure, match_prefix expects the first key to be less than or equal to the second" );
         return memcmp(shorter.data(), longer.data(), shorter.size()) == 0;
      }

      bool match(const rocksdb::Slice& lhs, const rocksdb::Slice& rhs) {
         return lhs.size() == rhs.size() &&
                memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
      }

      b1::chain_kv::view::iterator find_lowerbound(name code, const rocksdb::Slice& prefix, const rocksdb::Slice& key) {
         b1::chain_kv::view::iterator chain_kv_iter{view, code.to_uint64_t(), prefix};
         chain_kv_iter.lower_bound(key.data(), key.size());
         return chain_kv_iter;
      }

      using found_keys = std::tuple<SecondaryKey, uint64_t, name>;
      struct fk_index {
         static constexpr std::size_t secondary = 0;
         static constexpr std::size_t primary = 1;
         static constexpr std::size_t payer = 2;
      };

      template<typename CharCont>
      std::optional<found_keys> find_matching_sec_prim(const b1::chain_kv::key_value& found_kv,
                                                       const CharCont& prefix) {
         SecondaryKey secondary_key;
         uint64_t primary_key;
         if (!backing_store::db_key_value_format::get_trailing_sec_prim_keys(found_kv.key, prefix, secondary_key, primary_key)) {
            // since this is not the desired secondary key entry, reject it
            return {};
         }

         account_name payer = payer_payload(found_kv.value).payer;
         return found_keys{ std::move(secondary_key), std::move(primary_key), std::move(payer) };
      }

      template<typename CharKey>
      std::optional<found_keys> find_matching_prim_sec(const b1::chain_kv::key_value& found_kv,
                                                       const CharKey& prefix) {
         SecondaryKey secondary_key;
         uint64_t primary_key;
         if (!backing_store::db_key_value_format::get_trailing_prim_sec_keys(found_kv.key, prefix, primary_key, secondary_key)) {
            // since this is not the desired secondary key entry, reject it
            return {};
         }

         account_name payer = payer_payload(found_kv.value).payer;
         return found_keys{ std::move(secondary_key), std::move(primary_key), std::move(payer) };
      }

      enum class bound_type { lower, upper };
      int bound_secondary( name code, name scope, name table, bound_type bt, SecondaryKey& secondary, uint64_t& primary ) {
         const char* const bt_str = (bt == bound_type::upper) ? "upper" : "lower";
         prefix_bundle secondary_key = get_secondary_slice_in_table(scope, table, secondary);
         // setting the "key space" to be the whole table, so that we either get a match or another key for this table
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(code, secondary_key.prefix, secondary_key.key);

         auto found_kv = chain_kv_iter.get_kv();
         if (!found_kv) {
            // no keys for this entire table
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         const rocksdb::Slice type_slice = db_key_value_format::prefix_type_slice(secondary_key.key);
         // verify if the key that was found contains a secondary key of this type
         if (!match_prefix(type_slice, found_kv->key)) {
            return table_ei;
         }
         else if (bt == bound_type::upper) {
            // need to advance till we get to a different secondary key
            while (match_prefix(secondary_key.key, found_kv->key)) {
               ++chain_kv_iter;
               found_kv = chain_kv_iter.get_kv();
               EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                           "invariant failure in db_${d}_${bt_str}bound_secondary, found a secondary key that matched the "
                           "requested key, but no other keys returned following it and there should at least be a "
                           "table key marking the end of the scope/table's key space",
                           ("d", helper.desc())("bt_str", bt_str));
               if (!match_prefix(type_slice, found_kv->key)) {
                  return table_ei;
               }
            }
         }

         SecondaryKey secondary_ret {};
         uint64_t primary_ret = 0;
         const bool valid_key = db_key_value_format::get_trailing_sec_prim_keys<SecondaryKey, rocksdb::Slice, rocksdb::Slice>(
               found_kv->key, type_slice, secondary_ret, primary_ret);
         EOS_ASSERT( valid_key, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_${bt_str}bound_secondary, since the type slice matched, the trailing "
                     "keys should have been extracted", ("d", helper.desc())("bt_str",bt_str));

         // since this method cannot control that the input for primary and secondary having overlapping memory
         // setting primary 1st and secondary 2nd to maintain the same results as chainbase
         primary = primary_ret;
         secondary = secondary_ret;

         return iter_store.add({ .table_ei = table_ei, .secondary = secondary_ret, .primary = primary_ret,
                                 .payer = payer_payload(found_kv->value.data(), found_kv->value.size()).payer });
      }

      int bound_primary( name code, name scope, name table, uint64_t primary, bound_type bt ){
         const bytes prim_to_sec_key = db_key_value_format::create_prefix_primary_to_secondary_key<SecondaryKey>(scope, table, primary);
         const rocksdb::Slice key = {prim_to_sec_key.data(), prim_to_sec_key.size()};
         const rocksdb::Slice prefix = db_key_value_format::prefix_type_slice(prim_to_sec_key);
         b1::chain_kv::view::iterator chain_kv_iter = find_lowerbound(code, prefix, key);
         auto found_kv = chain_kv_iter.get_kv();
         // check if nothing remains in the table database
         if (!found_kv) {
            return iter_store.invalid_iterator();
         }
         else if (bt == bound_type::upper && match_prefix(key, found_kv->key)) {
            ++chain_kv_iter;
            found_kv = chain_kv_iter.get_kv();
            if (!found_kv) {
               return iter_store.invalid_iterator();
            }
         }

         auto found_keys = find_matching_prim_sec(*found_kv, prefix);
         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         if (!found_keys) {
            return table_ei;
         }
         return iter_store.add({ .table_ei = table_ei, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                 .primary = std::move(std::get<fk_index::primary>(*found_keys)),
                                 .payer = std::move(std::get<fk_index::payer>(*found_keys))});
      }

      db_key_value_iter_store<SecondaryKey> iter_store;
      using iter_obj = typename db_key_value_iter_store<SecondaryKey>::secondary_obj_type;
      secondary_helper<SecondaryKey> helper;
   };

}}} // ns eosio::chain::backing_store
