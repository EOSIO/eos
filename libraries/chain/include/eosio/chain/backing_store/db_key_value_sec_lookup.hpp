#pragma once

#include <eosio/chain/backing_store/chain_kv_payer.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace backing_store {

   template<typename SecondaryKey>
   struct secondary_helper;

   template<>
   struct secondary_helper<uint64_t> {
      const char* desc() { return "idx64"; }
      eosio::session::shared_bytes value(const uint64_t& sec_key, name payer) {
         return payer_payload(payer, nullptr, 0).as_payload();
      }
      void extract(const eosio::session::shared_bytes& payload, uint64_t& sec_key, name& payer) {
         payer_payload pp(payload);
         payer = pp.payer;
         EOS_ASSERT( pp.value_size == 0, db_rocksdb_invalid_operation_exception,
                     "Payload should not have anything more than the payer");
      }
      constexpr inline uint64_t overhead() { return config::billable_size_v<index64_object>;}
   };

   template<>
   struct secondary_helper<eosio::chain::uint128_t> {
      const char* desc() { return "idx128"; }
      eosio::session::shared_bytes value(const eosio::chain::uint128_t& sec_key, name payer) {
         return payer_payload(payer, nullptr, 0).as_payload();
      }
      void extract(const eosio::session::shared_bytes& payload, eosio::chain::uint128_t& sec_key, name& payer) {
         payer_payload pp(payload);
         payer = pp.payer;
         EOS_ASSERT( pp.value_size == 0, db_rocksdb_invalid_operation_exception,
                     "Payload should not have anything more than the payer");
      }
      constexpr inline uint64_t overhead() { return config::billable_size_v<index128_object>;}
   };

   template<>
   struct secondary_helper<eosio::chain::key256_t> {
      const char* desc() { return "idx256"; }
      eosio::session::shared_bytes value(const eosio::chain::key256_t& sec_key, name payer) {
         return payer_payload(payer, nullptr, 0).as_payload();
      }
      void extract(const eosio::session::shared_bytes& payload, eosio::chain::key256_t& sec_key, name& payer) {
         payer_payload pp(payload);
         payer = pp.payer;
         EOS_ASSERT( pp.value_size == 0, db_rocksdb_invalid_operation_exception,
                     "Payload should not have anything more than the payer");
      }
      constexpr inline uint64_t overhead() { return config::billable_size_v<index256_object>;}
   };

   template<>
   struct secondary_helper<float64_t> {
      const char* desc() { return "idx_double"; }
      eosio::session::shared_bytes value(const float64_t& sec_key, name payer) {
         constexpr static auto value_size = sizeof(sec_key);
         char value_buf[value_size];
         memcpy(value_buf, &sec_key, value_size);
         // float64_t stores off the original bit pattern since the key pattern has to lose the differentiation
         // between +0.0 and -0.0
         return payer_payload(payer, value_buf, value_size).as_payload();
      }
      void extract(const eosio::session::shared_bytes& payload, float64_t& sec_key, name& payer) {
         payer_payload pp(payload);
         EOS_ASSERT( pp.value_size == sizeof(sec_key), db_rocksdb_invalid_operation_exception,
                     "Payload does not contain the expected float64_t exact representation");
         payer = pp.payer;
         // see note on value(...) method above
         memcpy(&sec_key, pp.value, pp.value_size);
      }
      constexpr inline uint64_t overhead() { return config::billable_size_v<index_double_object>;}
   };

   template<>
   struct secondary_helper<float128_t> {
      const char* desc() { return "idx_long_double"; }
      eosio::session::shared_bytes value(const float128_t& sec_key, name payer) {
         constexpr static auto value_size = sizeof(sec_key);
         // NOTE: this is not written in a way that sorting on value would work, but it is just used for storage
         char value_buf[value_size];
         memcpy(value_buf, &sec_key, value_size);
         // float128_t stores off the original bit pattern since the key pattern has to lose the differentiation
         // between +0.0 and -0.0
         return payer_payload(payer, value_buf, value_size).as_payload();
      }
      void extract(const eosio::session::shared_bytes& payload, float128_t& sec_key, name& payer) {
         payer_payload pp(payload);
         EOS_ASSERT( pp.value_size == sizeof(sec_key), db_rocksdb_invalid_operation_exception,
                     "Payload does not contain the expected float128_t exact representation");
         payer = pp.payer;
         // see note on value(...) method above
         memcpy(&sec_key, pp.value, pp.value_size);
      }
      constexpr inline uint64_t overhead() { return config::billable_size_v<index_long_double_object>;}
   };

   template<typename SecondaryKey >
   class db_key_value_sec_lookup : public db_key_value_any_lookup
   {
   public:
      db_key_value_sec_lookup( db_context& p, session_variant_type& s ) : db_key_value_any_lookup(p, s) {}

      int store( name scope, name table, const account_name& payer,
                 uint64_t id, const SecondaryKey& secondary ) {
         EOS_ASSERT( payer != account_name(), invalid_table_payer, "must specify a valid account to pay for new record" );

         const sec_pair_bundle secondary_key = get_secondary_slices_in_secondaries(parent.receiver, scope, table, secondary, id);

         add_table_if_needed(scope, table, payer);

         auto old_value = current_session.read(secondary_key.full_secondary_key);

         EOS_ASSERT( !old_value, db_rocksdb_invalid_operation_exception, "db_${d}_store called with pre-existing key", ("d", helper.desc()));

         // identify if this primary key already has a secondary key of this type
         auto session_iter = current_session.lower_bound(secondary_key.prefix_primary_to_sec_key);
         EOS_ASSERT( !match_prefix(secondary_key.prefix_primary_to_sec_key, session_iter), db_rocksdb_invalid_operation_exception,
                     "db_${d}_store called, but primary key: ${primary} already has a secondary key of this type",
                     ("d", helper.desc())("primary", id));

         set_value(secondary_key.full_secondary_key, helper.value(secondary, payer));
         set_value(secondary_key.full_primary_to_sec_key, useless_value);

         std::string event_id;
         if (parent.context.control.get_deep_mind_logger() != nullptr) {
            event_id = db_context::table_event(parent.receiver, scope, table, name(id));
         }

         parent.context.update_db_usage( payer, helper.overhead(), backing_store::db_context::secondary_add_trace(parent.context.get_action_id(), std::move(event_id)) );

         const unique_table t { parent.receiver, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         return iter_store.add({ .table_ei = table_ei, .secondary = secondary, .primary = id, .payer = payer });
      }

      void remove( int iterator ) {
         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);
         EOS_ASSERT( table.contract == parent.receiver, table_access_violation, "db access violation" );

         const sec_pair_bundle secondary_key = get_secondary_slices_in_secondaries(parent.receiver, table.scope, table.table, key_store.secondary, key_store.primary);
         auto old_value = current_session.read(secondary_key.full_secondary_key);

         EOS_ASSERT( old_value, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_remove, iter store found to update but nothing in database", ("d", helper.desc()));

         auto session_iter = current_session.lower_bound(secondary_key.prefix_primary_to_sec_key);
         EOS_ASSERT( match_prefix(secondary_key.full_primary_to_sec_key, session_iter), db_rocksdb_invalid_operation_exception,
                     "db_${d}_remove called, but primary key: ${primary} didn't have a primary to secondary key",
                     ("d", helper.desc())("primary", key_store.primary));

         std::string event_id;
         if (parent.context.control.get_deep_mind_logger() != nullptr) {
            event_id = db_context::table_event(table.contract, table.scope, table.table, name(key_store.primary));
         }

         parent.context.update_db_usage( key_store.payer, -( helper.overhead() ), db_context::secondary_rem_trace(parent.context.get_action_id(), std::move(event_id)) );

         current_session.erase(secondary_key.full_secondary_key);
         current_session.erase(secondary_key.full_primary_to_sec_key);
         remove_table_if_empty(secondary_key.full_secondary_key);
         iter_store.remove(iterator);
      }

      void update( int iterator, account_name payer, const SecondaryKey& secondary ) {
         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);
         EOS_ASSERT( table.contract == parent.receiver, table_access_violation, "db access violation" );

         const sec_pair_bundle secondary_key = get_secondary_slices_in_secondaries(parent.receiver, table.scope, table.table, key_store.secondary, key_store.primary);
         auto old_value = current_session.read(secondary_key.full_secondary_key);

         secondary_helper<SecondaryKey> helper;
         EOS_ASSERT( old_value, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_remove, iter store found to update but nothing in database", ("d", helper.desc()));

         // identify if this primary key already has a secondary key of this type
         auto primary_sec_uesless_value = current_session.read(secondary_key.full_primary_to_sec_key);
         EOS_ASSERT( primary_sec_uesless_value, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_update, the secondary to primary lookup key was found, but not the "
                     "primary to secondary lookup key", ("d", helper.desc()));

         if( payer == account_name() ) payer = key_store.payer;

         auto& context = parent.context;
         std::string event_id;
         if (context.control.get_deep_mind_logger() != nullptr) {
            event_id = backing_store::db_context::table_event(table.contract, table.scope, table.table, name(key_store.primary));
         }

         if( key_store.payer != payer ) {
            context.update_db_usage( key_store.payer, -helper.overhead(), backing_store::db_context::secondary_update_rem_trace(context.get_action_id(), std::string(event_id)) );
            context.update_db_usage( payer, helper.overhead(), backing_store::db_context::secondary_update_add_trace(context.get_action_id(), std::move(event_id)) );
         }

         // if the secondary value is different, remove the old key and add the new key
         if (secondary != key_store.secondary) {
            // remove the secondary (to primary) key and the primary to secondary key
            current_session.erase(secondary_key.full_secondary_key);
            current_session.erase(secondary_key.full_primary_to_sec_key);

            // store the new secondary (to primary) key
            const auto new_secondary_keys = db_key_value_format::create_secondary_key_pair(table.scope, table.table, secondary, key_store.primary);
            set_value(db_key_value_format::create_full_key(new_secondary_keys.secondary_key, parent.receiver), helper.value(secondary, payer));

            // store the new primary to secondary key
            set_value(db_key_value_format::create_full_key(new_secondary_keys.primary_to_secondary_key, parent.receiver), useless_value);
         }
         else {
            // no key values have changed, so can use the old key (and no change to secondary to primary key)
            set_value(secondary_key.full_primary_to_sec_key, helper.value(secondary, payer));
         }
         iter_store.swap(iterator, secondary, payer);
      }

      int find_secondary( name code, name scope, name table, const SecondaryKey& secondary, uint64_t& primary ) {
         prefix_bundle secondary_key = get_secondary_slice_in_table(code, scope, table, secondary);
         auto session_iter = current_session.lower_bound(secondary_key.full_key);

         // if we don't get any match for this table, then return the invalid iterator
         if (!match_prefix(secondary_key.prefix_key, session_iter)) {
            // no keys for this entire table
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         // verify if the key that was found contains this secondary key
         if (!match_prefix(secondary_key.full_key, session_iter)) {
            return table_ei;
         }

         const auto full_slice = db_key_value_format::extract_legacy_slice((*session_iter).first);
         const auto prefix_secondary_slice = db_key_value_format::extract_legacy_slice(secondary_key.full_key);
         const bool valid_key = db_key_value_format::get_trailing_primary_key(full_slice, prefix_secondary_slice, primary);
         EOS_ASSERT( valid_key, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_find_secondary, key portions were identical but "
                     "get_trailing_primary_key indicated that it couldn't extract", ("d", helper.desc()));

         return iter_store.add({ .table_ei = table_ei, .secondary = secondary, .primary = primary,
                                 .payer = payer_payload(*(*session_iter).second).payer });
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

         prefix_bundle secondary_key = get_secondary_slice_in_secondaries(table.contract, table.scope, table.table, key_store.secondary, key_store.primary);
         auto session_iter = current_session.lower_bound(secondary_key.full_key);
         EOS_ASSERT( match(secondary_key.full_key, session_iter), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, found iterator in iter store but didn't find "
                     "any entry in the database (no secondary keys of this specific type)", ("d", helper.desc()));
         ++session_iter;
         if (!match_prefix(secondary_key.prefix_key, session_iter)) {
            return key_store.table_ei;
         }

         SecondaryKey secondary;
         const bool valid_key = db_key_value_format::get_trailing_sec_prim_keys((*session_iter).first, secondary_key.prefix_key, secondary, primary);
         EOS_ASSERT( valid_key, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, since the type slice matched, the trailing "
                     "primary key should have been extracted", ("d", helper.desc()));

         return iter_store.add({ .table_ei = key_store.table_ei, .secondary = secondary, .primary = primary,
                                 .payer = payer_payload(*(*session_iter).second).payer });
      }

      int previous_secondary( int iterator, uint64_t& primary ) {
         if( iterator < iter_store.invalid_iterator() ) {
            const backing_store::unique_table* table = iter_store.find_table_by_end_iterator(iterator);
            EOS_ASSERT( table, invalid_table_iterator, "not a valid end iterator" );
            constexpr static auto kt = db_key_value_format::derive_secondary_key_type<SecondaryKey>();
            const bytes legacy_type_key = db_key_value_format::create_prefix_type_key(table->scope, table->table, kt);
            const shared_bytes type_prefix = db_key_value_format::create_full_key(legacy_type_key, table->contract);
            const shared_bytes next_type_prefix = type_prefix.next();
            auto session_iter = current_session.lower_bound(next_type_prefix);
            --session_iter; // move back to the last secondary key of this type (if there is one)
            // check if there is any secondary key of this type
            if (!match_prefix(type_prefix, session_iter)) {
               return iter_store.invalid_iterator();
            }

            auto found_keys = find_matching_sec_prim(*session_iter, type_prefix);
            if (!found_keys) {
               return iter_store.invalid_iterator();
            }
            primary = std::get<fk_index::primary>(*found_keys);
            return iter_store.add({ .table_ei = iterator, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                    .primary = std::move(std::get<fk_index::primary>(*found_keys)),
                                    .payer = std::move(std::get<fk_index::payer>(*found_keys))});
         }

         const iter_obj& key_store = iter_store.get(iterator);
         const unique_table& table = iter_store.get_table(key_store);

         prefix_bundle secondary_key = get_secondary_slice_in_secondaries(table.contract, table.scope, table.table, key_store.secondary, key_store.primary);
         auto session_iter = current_session.lower_bound(secondary_key.full_key);
         EOS_ASSERT( match(secondary_key.full_key, session_iter), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_secondary, found iterator in iter store but didn't find its "
                     "matching entry in the database", ("d", helper.desc()));
         --session_iter;
         if (!match_prefix(secondary_key.prefix_key, session_iter)) {
            return key_store.table_ei;
         }

         auto found_keys = find_matching_sec_prim(*session_iter, secondary_key.prefix_key);
         EOS_ASSERT( found_keys, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_secondary, since the type slice matched, the trailing "
                     "primary key should have been extracted", ("d", helper.desc()));
         primary = std::get<fk_index::primary>(*found_keys);

         return iter_store.add({ .table_ei = key_store.table_ei, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                 .primary = primary, .payer = std::move(std::get<fk_index::payer>(*found_keys)) });
      }

      int find_primary( name code, name scope, name table, SecondaryKey& secondary, uint64_t primary ) {
         const bytes legacy_prim_to_sec_key = db_key_value_format::create_prefix_primary_to_secondary_key<SecondaryKey>(scope, table, primary);
         const shared_bytes key = db_key_value_format::create_full_key(legacy_prim_to_sec_key, code);
         const shared_bytes prefix = db_key_value_format::create_full_key_prefix(key, end_of_prefix::pre_type);
         auto session_iter = current_session.lower_bound(key);
         // check if nothing remains in the primary table space
         if (!match_prefix(prefix, session_iter)) {
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);

         // if this is not of the same type and primary key
         if (!backing_store::db_key_value_format::get_trailing_secondary_key((*session_iter).first, key, secondary)) {
            // since this is not the desired secondary key entry, reject it
            return table_ei;
         }

         const bytes legacy_secondary_key = db_key_value_format::create_secondary_key(scope, table, secondary, primary);
         const shared_bytes secondary_key = db_key_value_format::create_full_key(legacy_secondary_key, code);
         const auto old_value = current_session.read(secondary_key);
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
         const shared_bytes key = db_key_value_format::create_full_key(prim_to_sec_key, table.contract);
         const shared_bytes sec_type_prefix = db_key_value_format::create_full_key_prefix(key, end_of_prefix::at_prim_to_sec_type);
         auto session_iter = current_session.lower_bound(key);
         // This key has to exist, since it is cached in our iterator store
         EOS_ASSERT( match(key, session_iter), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_primary, found iterator in iter store but didn't find its "
                     "matching entry in the database", ("d", helper.desc()));
         ++session_iter;
         if (!match_prefix(sec_type_prefix, session_iter)) {
            return key_store.table_ei;
         }

         auto found_keys = find_matching_prim_sec(*session_iter, sec_type_prefix);
         EOS_ASSERT( found_keys, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_next_primary, since the type slice matched, the trailing "
                     "keys should have been extracted", ("d", helper.desc()));
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

            // create the key pointing to the start of the primary to secondary keys of this type, and then increment it to be past the end
            const shared_bytes type_prefix = db_key_value_format::create_full_key(types_key, table->contract);
            const shared_bytes next_type_prefix = type_prefix.next();
            auto session_iter = current_session.lower_bound(next_type_prefix); //upperbound of this primary_to_sec key type
            --session_iter; // move back to the last primary key of this type (if there is one)
            // check if no keys exist for this type
            if (!match_prefix(type_prefix, session_iter)) {
               return iter_store.invalid_iterator();
            }
            auto found_keys = find_matching_prim_sec(*session_iter, type_prefix);
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
         const shared_bytes key = db_key_value_format::create_full_key(prim_to_sec_key, table.contract);
         const shared_bytes sec_type_prefix = db_key_value_format::create_full_key_prefix(key, end_of_prefix::at_prim_to_sec_type);
         auto session_iter = current_session.lower_bound(key);
         EOS_ASSERT( match(key, session_iter), db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_primary, found iterator in iter store but didn't find its "
                     "matching entry in the database", ("d", helper.desc()));
         --session_iter;
         // determine if we are still in the range of this primary_to_sec key type
         if (!match_prefix(sec_type_prefix, session_iter)) {
            return iter_store.invalid_iterator();
         }

         auto found_keys = find_matching_prim_sec(*session_iter, sec_type_prefix);
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
         sec_pair_bundle(const b1::chain_kv::bytes& composite_sec_key, const b1::chain_kv::bytes& composite_prim_to_sec_key, name code)
         : full_secondary_key(db_key_value_format::create_full_key(composite_sec_key, code)),
           full_primary_to_sec_key(db_key_value_format::create_full_key(composite_prim_to_sec_key, code)),
           prefix_secondary_key(db_key_value_format::create_full_key_prefix(full_secondary_key, end_of_prefix::at_type)),
           prefix_primary_to_sec_key(db_key_value_format::create_full_key_prefix(full_primary_to_sec_key, end_of_prefix::at_prim_to_sec_primary_key)) {

         }

         shared_bytes full_secondary_key;
         shared_bytes full_primary_to_sec_key;
         shared_bytes prefix_secondary_key;       // points to the type prefix for composite_key
         shared_bytes prefix_primary_to_sec_key;  // points to the primary key in full_primary_to_sec_key (all except the secondary key)
      };

      // gets a prefix that allows for only this secondary key iterators, and the primary to secondary key, with a prefix key to the secondary key type
      static sec_pair_bundle get_secondary_slices_in_secondaries(name code, name scope, name table, const SecondaryKey& secondary, uint64_t primary) {
         auto secondary_keys = db_key_value_format::create_secondary_key_pair(scope, table, secondary, primary);
         return { secondary_keys.secondary_key, secondary_keys.primary_to_secondary_key, code };
      }

      // gets a specific secondary key without the trailing primary key, and will allow for only secondaries of this type (prefix)
      static prefix_bundle get_secondary_slice_in_secondaries(name code, name scope, name table, const SecondaryKey& secondary) {
         bytes secondary_key = db_key_value_format::create_prefix_secondary_key(scope, table, secondary);
         return { secondary_key, end_of_prefix::at_type, code };
      }

      // gets a specific secondary and primary keys, and will allow for only secondaries of this type (prefix)
      static prefix_bundle get_secondary_slice_in_secondaries(name code, name scope, name table, const SecondaryKey& secondary, uint64_t primary) {
         bytes secondary_key = db_key_value_format::create_secondary_key(scope, table, secondary, primary);
         return { secondary_key, end_of_prefix::at_type, code };
      }

      // gets a prefix that allows for a specific secondary key without the trailing primary key, but will allow for all iterators in the table
      static prefix_bundle get_secondary_slice_in_table(name code, name scope, name table, const SecondaryKey& secondary) {
         bytes secondary_key = db_key_value_format::create_prefix_secondary_key(scope, table, secondary);
         return { secondary_key, end_of_prefix::pre_type, code };
      }

      void set_value(const shared_bytes& full_key, const shared_bytes& payload) {
         current_session.write(full_key, payload);
      }

   private:
      using found_keys = std::tuple<SecondaryKey, uint64_t, name>;
      struct fk_index {
         static constexpr std::size_t secondary = 0;
         static constexpr std::size_t primary = 1;
         static constexpr std::size_t payer = 2;
      };

      template<typename CharCont>
      std::optional<found_keys> find_matching_sec_prim(const std::pair<shared_bytes, std::optional<shared_bytes> >& found_kv,
                                                       const CharCont& prefix) {
         SecondaryKey secondary_key;
         uint64_t primary_key;
         if (!backing_store::db_key_value_format::get_trailing_sec_prim_keys(found_kv.first, prefix, secondary_key, primary_key)) {
            // since this is not the desired secondary key entry, reject it
            return {};
         }

         EOS_ASSERT( found_kv.second, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_previous_secondary, the secondary key was found but it had no value "
                     "(the payer) stored for it", ("d", helper.desc()));
         account_name payer = payer_payload(*found_kv.second).payer;
         return found_keys{ std::move(secondary_key), std::move(primary_key), std::move(payer) };
      }

      template<typename CharKey>
      std::optional<found_keys> find_matching_prim_sec(const std::pair<shared_bytes, std::optional<shared_bytes>>& found_kv,
                                                       const shared_bytes& prefix) {
         SecondaryKey secondary_key;
         uint64_t primary_key;
         if (!backing_store::db_key_value_format::get_trailing_prim_sec_keys(found_kv.first, prefix, primary_key, secondary_key)) {
            // since this is not the desired secondary key entry, reject it
            return {};
         }

         auto full_secondary_key = backing_store::db_key_value_format::create_secondary_key_from_primary_to_sec_key<SecondaryKey>(found_kv.first);
         auto session_iter = current_session.read(full_secondary_key);
         EOS_ASSERT( match(found_kv.first, session_iter), db_rocksdb_invalid_operation_exception,
                     "invariant failure in find_matching_prim_sec, the primary to sec key was found in the database, "
                     "but no secondary to primary key");
         EOS_ASSERT( (*session_iter)->second, db_rocksdb_invalid_operation_exception,
                     "invariant failure in find_matching_prim_sec, the secondary key was found but it had no value "
                     "(the payer) stored for it");
         account_name payer = payer_payload(*(*session_iter)->second).payer;
         return found_keys{ std::move(secondary_key), std::move(primary_key), std::move(payer) };
      }

      enum class bound_type { lower, upper };
      const char* as_string(bound_type bt) { return (bt == bound_type::upper) ? "upper" : "lower"; }
      int bound_secondary( name code, name scope, name table, bound_type bt, SecondaryKey& secondary, uint64_t& primary ) {
         prefix_bundle secondary_key = get_secondary_slice_in_table(code, scope, table, secondary);
         auto session_iter = current_session.lower_bound(secondary_key.full_key);
         // setting the "key space" to be the whole table, so that we either get a match or another key for this table
         if (!match_prefix(secondary_key.prefix_key, session_iter)) {
            // no keys for this entire table
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);
         const auto type_prefix = db_key_value_format::create_full_key_prefix(secondary_key.full_key, end_of_prefix::at_type);
         // verify if the key that was found contains a secondary key of this type
         if (!match_prefix(type_prefix, session_iter)) {
            return table_ei;
         }
         else if (bt == bound_type::upper) {
            // need to advance till we get to a different secondary key
            while (match_prefix(secondary_key.full_key, session_iter)) {
               ++session_iter;
               if (!match_prefix(type_prefix, session_iter)) {
                  // no more secondary keys of this type
                  return table_ei;
               }
            }
         }

         SecondaryKey secondary_ret {};
         uint64_t primary_ret = 0;
         const bool valid_key = db_key_value_format::get_trailing_sec_prim_keys<SecondaryKey>(
               (*session_iter).first, type_prefix, secondary_ret, primary_ret);
         EOS_ASSERT( valid_key, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_${bt_str}bound_secondary, since the type slice matched, the trailing "
                     "keys should have been extracted", ("d", helper.desc())("bt_str",as_string(bt)));

         // since this method cannot control that the input for primary and secondary having overlapping memory
         // setting primary 1st and secondary 2nd to maintain the same results as chainbase
         primary = primary_ret;
         secondary = secondary_ret;

         return iter_store.add({ .table_ei = table_ei, .secondary = secondary_ret, .primary = primary_ret,
                                 .payer = payer_payload(*(*session_iter).second).payer });
      }

      int bound_primary( name code, name scope, name table, uint64_t primary, bound_type bt ){
         const bytes prim_to_sec_key = db_key_value_format::create_prefix_primary_to_secondary_key<SecondaryKey>(scope, table, primary);
         const shared_bytes key = db_key_value_format::create_full_key(prim_to_sec_key, code);
         // use the primary to secondary key type to only retrieve secondary keys of SecondaryKey type
         const shared_bytes sec_type_prefix = db_key_value_format::create_full_key_prefix(key, end_of_prefix::at_prim_to_sec_type);
         auto session_iter = current_session.lower_bound(key);
         // check if nothing remains in the table database
         if (!match_prefix(sec_type_prefix, session_iter)) {
            return iter_store.invalid_iterator();
         }

         const unique_table t { code, scope, table };
         const auto table_ei = iter_store.cache_table(t);

         if (bt == bound_type::upper && match_prefix(key, session_iter)) {
            ++session_iter;
            if (!match_prefix(sec_type_prefix, session_iter)) {
               return table_ei;
            }
         }

         auto found_keys = find_matching_prim_sec(*session_iter, sec_type_prefix);
         EOS_ASSERT( found_keys, db_rocksdb_invalid_operation_exception,
                     "invariant failure in db_${d}_${bt_str}bound_secondary, since the type slice matched, the trailing "
                     "keys should have been extracted", ("d", helper.desc())("bt_str",as_string(bt)));

         return iter_store.add({ .table_ei = table_ei, .secondary = std::move(std::get<fk_index::secondary>(*found_keys)),
                                 .primary = std::move(std::get<fk_index::primary>(*found_keys)),
                                 .payer = std::move(std::get<fk_index::payer>(*found_keys))});
      }

      db_key_value_iter_store<SecondaryKey> iter_store;
      using iter_obj = typename db_key_value_iter_store<SecondaryKey>::secondary_obj_type;
      secondary_helper<SecondaryKey> helper;
   };

}}} // ns eosio::chain::backing_store
