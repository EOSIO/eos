#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/db_key_value_iter_store.hpp>
#include <eosio/chain/backing_store/db_key_value_any_lookup.hpp>
#include <eosio/chain/backing_store/db_key_value_sec_lookup.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>
#include <b1/chain_kv/chain_kv.hpp>
#include <eosio/chain/combined_database.hpp>
#include <b1/session/rocks_session.hpp>

namespace eosio { namespace chain { namespace backing_store {

   class db_context_rocksdb : public db_context {
   public:
      using prim_key_iter_type = secondary_key<uint64_t>;
      using session_type = eosio::session::session<eosio::session::session<eosio::session::rocksdb_t>>;
      using shared_bytes = eosio::session::shared_bytes;

      db_context_rocksdb(apply_context& context, name receiver, session_type& session);

      ~db_context_rocksdb() override;

      int32_t db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char* value , size_t value_size) override;
      void db_update_i64(int32_t itr, account_name payer, const char* value , size_t value_size) override;
      void db_remove_i64(int32_t itr) override;
      int32_t db_get_i64(int32_t itr, char* value , size_t value_size) override;
      int32_t db_next_i64(int32_t itr, uint64_t& primary) override;
      int32_t db_previous_i64(int32_t itr, uint64_t& primary) override;
      int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override;
      int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override;
      int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) override;
      int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table) override;

      /**
       * interface for uint64_t secondary
       */
      int32_t db_idx64_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                     const uint64_t& secondary) override;
      void db_idx64_update(int32_t iterator, account_name payer, const uint64_t& secondary) override;
      void db_idx64_remove(int32_t iterator) override;
      int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint64_t& secondary,
                                      uint64_t& primary) override;
      int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                    uint64_t primary) override;
      int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                  uint64_t& primary) override;
      int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                  uint64_t& primary) override;
      int32_t db_idx64_end(uint64_t code, uint64_t scope, uint64_t table) override;
      int32_t db_idx64_next(int32_t iterator, uint64_t& primary) override;
      int32_t db_idx64_previous(int32_t iterator, uint64_t& primary) override;

      /**
       * interface for uint128_t secondary
       */
      int32_t db_idx128_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                              const uint128_t& secondary) override;
      void db_idx128_update(int32_t iterator, account_name payer, const uint128_t& secondary) override;
      void db_idx128_remove(int32_t iterator) override;
      int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t& secondary,
                                       uint64_t& primary) override;
      int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                     uint64_t primary) override;
      int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                   uint64_t& primary) override;
      int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                   uint64_t& primary) override;
      int32_t db_idx128_end(uint64_t code, uint64_t scope, uint64_t table) override;
      int32_t db_idx128_next(int32_t iterator, uint64_t& primary) override;
      int32_t db_idx128_previous(int32_t iterator, uint64_t& primary) override;

      /**
       * interface for 256-bit interger secondary
       */
      int32_t db_idx256_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                              const uint128_t* data) override;
      void db_idx256_update(int32_t iterator, account_name payer, const uint128_t* data) override;

      void db_idx256_remove(int32_t iterator) override;
      int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* data,
                                       uint64_t& primary) override;
      int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                     uint64_t primary) override;
      int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                   uint64_t& primary) override;
      int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                   uint64_t& primary) override;
      int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) override;
      int32_t db_idx256_next(int32_t iterator, uint64_t& primary) override;
      int32_t db_idx256_previous(int32_t iterator, uint64_t& primary) override;

      /**
       * interface for double secondary
       */
      int32_t db_idx_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                  const float64_t& secondary) override;
      void db_idx_double_update(int32_t iterator, account_name payer, const float64_t& secondary) override;
      void db_idx_double_remove(int32_t iterator) override;
      int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                           const float64_t& secondary, uint64_t& primary) override;
      int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                         uint64_t primary) override;
      int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                       uint64_t& primary) override;
      int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                       uint64_t& primary) override;
      int32_t db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table) override;
      int32_t db_idx_double_next(int32_t iterator, uint64_t& primary) override;
      int32_t db_idx_double_previous(int32_t iterator, uint64_t& primary) override;
      /**
       * interface for long double secondary
       */
      int32_t db_idx_long_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                       const float128_t& secondary) override;
      void db_idx_long_double_update(int32_t iterator, account_name payer, const float128_t& secondary) override;
      void db_idx_long_double_remove(int32_t iterator) override;
      int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                const float128_t& secondary, uint64_t& primary) override;
      int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table,
                                              float128_t& secondary, uint64_t primary) override;
      int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                            uint64_t& primary) override;
      int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                            uint64_t& primary) override;
      int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table) override;
      int32_t db_idx_long_double_next(int32_t iterator, uint64_t& primary) override;
      int32_t db_idx_long_double_previous(int32_t iterator, uint64_t& primary) override;

      // for primary keys in the iterator store, we don't care about secondary key
      static prim_key_iter_type primary_key_iter(int table_ei, uint64_t key, account_name payer);
      void swap(int iterator, account_name payer);

      // gets a prefix that allows for only primary key iterators
      static prefix_bundle get_primary_slice_in_primaries(name code, name scope, name table, uint64_t id);
      pv_bundle get_primary_key_value(name code, name scope, name table, uint64_t id);
      void set_value(const shared_bytes& primary_key, const payer_payload& pp);
      void update_db_usage( const account_name& payer, int64_t delta, const storage_usage_trace& trace );
      int32_t find_and_store_primary_key(const session_type::iterator& session_iter, int32_t table_ei,
                                         const shared_bytes& type_prefix, int32_t not_found_return,
                                         const char* calling_func, uint64_t& found_key);
      struct exact_iterator {
         bool                   valid = false;
         session_type::iterator itr;
         shared_bytes           type_prefix;
      };
      exact_iterator get_exact_iterator(name code, name scope, name table, uint64_t primary);

      enum class comp { equals, gte, gt};
      int32_t find_i64(name code, name scope, name table, uint64_t id, comp comparison);

      using uint128_t = eosio::chain::uint128_t;
      using key256_t = eosio::chain::key256_t;
      session_type&                        current_session;
      db_key_value_iter_store<uint64_t>    primary_iter_store;
      db_key_value_any_lookup              primary_lookup;
      db_key_value_sec_lookup<uint64_t>    sec_lookup_i64;
      db_key_value_sec_lookup<uint128_t>   sec_lookup_i128;
      db_key_value_sec_lookup<key256_t>    sec_lookup_i256;
      db_key_value_sec_lookup<float64_t>   sec_lookup_double;
      db_key_value_sec_lookup<float128_t>  sec_lookup_long_double;
      static constexpr uint64_t            noop_secondary = 0x0;
   }; // db_context_rocksdb

   db_context_rocksdb::db_context_rocksdb(apply_context& context, name receiver, session_type& session)
   : db_context( context, receiver ), current_session{ session }, primary_lookup(*this, session),
     sec_lookup_i64(*this, session), sec_lookup_i128(*this, session), sec_lookup_i256(*this, session),
     sec_lookup_double(*this, session), sec_lookup_long_double(*this, session) {}

   db_context_rocksdb::~db_context_rocksdb() {
   }

   int32_t db_context_rocksdb::db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char* value , size_t value_size) {
      EOS_ASSERT( payer != account_name(), invalid_table_payer, "must specify a valid account to pay for new record" );
      const name scope_name{scope};
      const name table_name{table};
      const auto old_key_value = get_primary_key_value(receiver, scope_name, table_name, id);

      EOS_ASSERT( !old_key_value.value, db_rocksdb_invalid_operation_exception, "db_store_i64 called with pre-existing key");

      primary_lookup.add_table_if_needed(old_key_value.full_key, payer);

      const payer_payload pp{payer, value, value_size};
      set_value(old_key_value.full_key, pp);

      const int64_t billable_size = static_cast<int64_t>(value_size + db_key_value_any_lookup::overhead);

      std::string event_id;
      auto dm_logger = context.control.get_deep_mind_logger();
      if (dm_logger != nullptr) {
         event_id = db_context::table_event(receiver, scope_name, table_name, name(id));
      }

      update_db_usage( payer, billable_size, db_context::row_add_trace(context.get_action_id(), event_id) );

      if (dm_logger != nullptr) {
         db_context::write_row_insert(*dm_logger, context.get_action_id(), receiver, scope_name, table_name, payer,
               name(id), value, value_size);
      }

      const unique_table t { receiver, scope_name, table_name };
      const auto table_ei = primary_iter_store.cache_table(t);
      return primary_iter_store.add(primary_key_iter(table_ei, id, payer));
   }

   void db_context_rocksdb::db_update_i64(int32_t itr, account_name payer, const char* value , size_t value_size) {
      const auto& key_store = primary_iter_store.get(itr);
      const auto& table_store = primary_iter_store.get_table(key_store);
      EOS_ASSERT( table_store.contract == receiver, table_access_violation, "db access violation" );
      const auto old_key_value = get_primary_key_value(receiver, table_store.scope, table_store.table, key_store.primary);

      EOS_ASSERT( old_key_value.value, db_rocksdb_invalid_operation_exception,
                  "invariant failure in db_update_i64, iter store found to update but nothing in database");

      // copy locally, since below the key_store memory will be changed
      const auto old_payer = key_store.payer;
      if (payer.empty()) {
         payer = old_payer;
      }

      const payer_payload pp{payer, value, value_size};
      set_value(old_key_value.full_key, pp);

      const auto old_value_actual_size = pp.value_size;

      std::string event_id;
      auto dm_logger = context.control.get_deep_mind_logger();
      if (dm_logger != nullptr) {
         event_id = db_context::table_event(table_store.contract, table_store.scope, table_store.table, name(key_store.primary));
      }

      const int64_t old_size = static_cast<int64_t>(old_value_actual_size + db_key_value_any_lookup::overhead);
      const int64_t new_size = static_cast<int64_t>(value_size + db_key_value_any_lookup::overhead);
      if( old_payer != payer ) {
         // refund the existing payer
         update_db_usage( old_payer, -(old_size), db_context::row_update_rem_trace(context.get_action_id(), event_id) );
         // charge the new payer
         update_db_usage( payer,  (new_size), db_context::row_update_add_trace(context.get_action_id(), event_id) );

         // swap the payer in the iterator store
         swap(itr, payer);
      } else if(old_size != new_size) {
         // charge/refund the existing payer the difference
         update_db_usage( old_payer, new_size - old_size, db_context::row_update_trace(context.get_action_id(), event_id) );
      }

      if (dm_logger != nullptr) {
         db_context::write_row_update(*dm_logger, context.get_action_id(), table_store.contract, table_store.scope,
         table_store.table, old_payer, payer, name(key_store.primary),
         old_key_value.value->data(), old_key_value.value->size(), value, value_size);
      }
   }

   void db_context_rocksdb::db_remove_i64(int32_t itr) {
      const auto& key_store = primary_iter_store.get(itr);
      const auto& table_store = primary_iter_store.get_table(key_store);
      EOS_ASSERT( table_store.contract == receiver, table_access_violation, "db access violation" );
      const auto old_key_value = get_primary_key_value(receiver, table_store.scope, table_store.table, key_store.primary);

      EOS_ASSERT( old_key_value.value, db_rocksdb_invalid_operation_exception,
      "invariant failure in db_remove_i64, iter store found to update but nothing in database");

      const auto old_payer = key_store.payer;

      std::string event_id;
      auto dm_logger = context.control.get_deep_mind_logger();
      if (dm_logger != nullptr) {
         event_id = db_context::table_event(table_store.contract, table_store.scope, table_store.table, name(key_store.primary));
      }

      payer_payload pp(*old_key_value.value);
      update_db_usage( old_payer,  -((int64_t)pp.value_size + db_key_value_any_lookup::overhead), db_context::row_rem_trace(context.get_action_id(), event_id) );

      if (dm_logger != nullptr) {
         db_context::write_row_remove(*dm_logger, context.get_action_id(), table_store.contract, table_store.scope,
                                      table_store.table, old_payer, name(key_store.primary), pp.value,
                                      pp.value_size);
      }

      current_session.erase(old_key_value.full_key);
      primary_lookup.remove_table_if_empty(old_key_value.full_key);

      primary_iter_store.remove(itr); // don't use key_store anymore
   }

   int32_t db_context_rocksdb::db_get_i64(int32_t itr, char* value , size_t value_size) {
      const auto& key_store = primary_iter_store.get(itr);
      const auto& table_store = primary_iter_store.get_table(key_store);
      const auto old_key_value = get_primary_key_value(table_store.contract, table_store.scope, table_store.table, key_store.primary);

      EOS_ASSERT( old_key_value.value, db_rocksdb_invalid_operation_exception,
                  "invariant failure in db_get_i64, iter store found to update but nothing in database");
      payer_payload pp {*old_key_value.value};
      const char* const actual_value = pp.value;
      const size_t actual_size = pp.value_size;
      if (value_size == 0) {
         return actual_size;
      }
      const size_t copy_size = std::min<size_t>(value_size, actual_size);
      memcpy( value, actual_value, copy_size );
      return copy_size;
   }

   int32_t db_context_rocksdb::db_next_i64(int32_t itr, uint64_t& primary) {
      if (itr < primary_iter_store.invalid_iterator()) return primary_iter_store.invalid_iterator(); // cannot increment past end iterator of table

      const auto& key_store = primary_iter_store.get(itr);
      const auto& table_store = primary_iter_store.get_table(key_store);
      auto exact =
            get_exact_iterator(table_store.contract, table_store.scope, table_store.table, key_store.primary);
      EOS_ASSERT( exact.valid, db_rocksdb_invalid_operation_exception,
                  "invariant failure in db_next_i64, iter store found to update but it does not exist in the database");
      auto& session_iter = exact.itr;
      auto& type_prefix = exact.type_prefix;
      ++session_iter;
      return find_and_store_primary_key(session_iter, key_store.table_ei, type_prefix,
                                        key_store.table_ei, __func__, primary);
   }

   int32_t db_context_rocksdb::db_previous_i64(int32_t itr, uint64_t& primary) {
      if( itr < primary_iter_store.invalid_iterator() ) { // is end iterator
         const backing_store::unique_table* table_store = primary_iter_store.find_table_by_end_iterator(itr);
         EOS_ASSERT( table_store, invalid_table_iterator, "not a valid end iterator" );
         if (current_session.begin() == current_session.end()) {
            // NOTE: matching chainbase functionality, if iterator store found, but no keys in db
            return primary_iter_store.invalid_iterator();
         }

         const auto primary_bounded_key =
               get_primary_slice_in_primaries(table_store->contract, table_store->scope, table_store->table,
                                              std::numeric_limits<uint64_t>::max());
         auto session_iter = current_session.lower_bound(primary_bounded_key.full_key);

         auto past_end = [&](const auto& iter) {
            return !primary_lookup.match_prefix(primary_bounded_key.prefix_key, iter);
         };
         // if we are at the end of the db or past the end of this table, then step back one
         if (past_end(session_iter)) {
            --session_iter;
            // either way, the iterator after our known table should have a primary key in this table as the previous iterator
            if (!past_end(session_iter) || session_iter == current_session.begin()) {
               // NOTE: matching chainbase functionality, if iterator store found, but no key in db for table
               return primary_iter_store.invalid_iterator();
            }
         }

         return find_and_store_primary_key(session_iter, primary_iter_store.get_end_iterator_by_table(*table_store),
                                           primary_bounded_key.prefix_key, primary_iter_store.invalid_iterator(),
                                           __func__, primary);
      }

      const auto& key_store = primary_iter_store.get(itr);
      const backing_store::unique_table& table_store = primary_iter_store.get_table(key_store);
      const auto slice_primary_key = get_primary_slice_in_primaries(table_store.contract, table_store.scope, table_store.table, key_store.primary);
      auto exact = get_exact_iterator(table_store.contract, table_store.scope, table_store.table, key_store.primary);
      auto& session_iter = exact.itr;
      // if we didn't find the key, or know that we cannot decrement the iterator, then return the invalid iterator handle
      if (!exact.valid || session_iter == current_session.begin()) {
         return primary_iter_store.invalid_iterator();
      }

      auto& type_prefix = exact.type_prefix;
      --session_iter;
      return find_and_store_primary_key(session_iter, key_store.table_ei, type_prefix,
                                        primary_iter_store.invalid_iterator(), __func__, primary);
   }

   int32_t db_context_rocksdb::db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
      return find_i64(name{code}, name{scope}, name{table}, id, comp::equals);
   }

   int32_t db_context_rocksdb::db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
      return find_i64(name{code}, name{scope}, name{table}, id, comp::gte);
   }

   int32_t db_context_rocksdb::db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) {
      return find_i64(name{code}, name{scope}, name{table}, id, comp::gt);
   }

   int32_t db_context_rocksdb::db_end_i64(uint64_t code, uint64_t scope, uint64_t table) {
      return primary_lookup.get_end_iter(name{code}, name{scope}, name{table}, primary_iter_store);
   }

   /**
    * interface for uint64_t secondary
    */
   int32_t db_context_rocksdb::db_idx64_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                              const uint64_t& secondary) {
      return sec_lookup_i64.store(name{scope}, name{table}, payer, id, secondary);
   }

   void db_context_rocksdb::db_idx64_update(int32_t iterator, account_name payer, const uint64_t& secondary) {
      sec_lookup_i64.update(iterator, payer, secondary);
   }

   void db_context_rocksdb::db_idx64_remove(int32_t iterator) {
      sec_lookup_i64.remove(iterator);
   }

   int32_t db_context_rocksdb::db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint64_t& secondary,
                                   uint64_t& primary) {
      return sec_lookup_i64.find_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                 uint64_t primary) {
      return sec_lookup_i64.find_primary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                               uint64_t& primary) {
      return sec_lookup_i64.lowerbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                               uint64_t& primary) {
      return sec_lookup_i64.upperbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx64_end(uint64_t code, uint64_t scope, uint64_t table) {
      return sec_lookup_i64.end_secondary(name{code}, name{scope}, name{table});
   }

   int32_t db_context_rocksdb::db_idx64_next(int32_t iterator, uint64_t& primary) {
      return sec_lookup_i64.next_secondary(iterator, primary);
   }

   int32_t db_context_rocksdb::db_idx64_previous(int32_t iterator, uint64_t& primary) {
      return sec_lookup_i64.previous_secondary(iterator, primary);
   }

   /**
    * interface for uint128_t secondary
    */
   int32_t db_context_rocksdb::db_idx128_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                           const uint128_t& secondary) {
      return sec_lookup_i128.store(name{scope}, name{table}, payer, id, secondary);
   }

   void db_context_rocksdb::db_idx128_update(int32_t iterator, account_name payer, const uint128_t& secondary) {
      sec_lookup_i128.update(iterator, payer, secondary);
   }

   void db_context_rocksdb::db_idx128_remove(int32_t iterator) {
      sec_lookup_i128.remove(iterator);
   }

   int32_t db_context_rocksdb::db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t& secondary,
                                    uint64_t& primary) {
      return sec_lookup_i128.find_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                  uint64_t primary) {
      return sec_lookup_i128.find_primary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                uint64_t& primary) {
      return sec_lookup_i128.lowerbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                uint64_t& primary) {
      return sec_lookup_i128.upperbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx128_end(uint64_t code, uint64_t scope, uint64_t table) {
      return sec_lookup_i128.end_secondary(name{code}, name{scope}, name{table});
   }

   int32_t db_context_rocksdb::db_idx128_next(int32_t iterator, uint64_t& primary) {
      return sec_lookup_i128.next_secondary(iterator, primary);
   }

   int32_t db_context_rocksdb::db_idx128_previous(int32_t iterator, uint64_t& primary) {
      return sec_lookup_i128.previous_secondary(iterator, primary);
   }

   /**
    * interface for 256-bit interger secondary
    */
   int32_t db_context_rocksdb::db_idx256_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                           const uint128_t* data) {
      const eosio::chain::key256_t& secondary = *reinterpret_cast<const eosio::chain::key256_t*>(data);
      return sec_lookup_i256.store(name{scope}, name{table}, payer, id, secondary);
   }

   void db_context_rocksdb::db_idx256_update(int32_t iterator, account_name payer, const uint128_t* data) {
      const eosio::chain::key256_t& secondary = *reinterpret_cast<const eosio::chain::key256_t*>(data);
      sec_lookup_i256.update(iterator, payer, secondary);
   }

   void db_context_rocksdb::db_idx256_remove(int32_t iterator) {
      sec_lookup_i256.remove(iterator);
   }

   int32_t db_context_rocksdb::db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* data,
                                    uint64_t& primary) {
      const eosio::chain::key256_t& secondary = *reinterpret_cast<const eosio::chain::key256_t*>(data);
      return sec_lookup_i256.find_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                  uint64_t primary) {
      eosio::chain::key256_t& secondary = *reinterpret_cast<eosio::chain::key256_t*>(data);
      return sec_lookup_i256.find_primary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                uint64_t& primary) {
      eosio::chain::key256_t& secondary = *reinterpret_cast<eosio::chain::key256_t*>(data);
      return sec_lookup_i256.lowerbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                uint64_t& primary) {
      eosio::chain::key256_t& secondary = *reinterpret_cast<eosio::chain::key256_t*>(data);
      return sec_lookup_i256.upperbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) {
      return sec_lookup_i256.end_secondary(name{code}, name{scope}, name{table});
   }

   int32_t db_context_rocksdb::db_idx256_next(int32_t iterator, uint64_t& primary) {
      return sec_lookup_i256.next_secondary(iterator, primary);
   }

   int32_t db_context_rocksdb::db_idx256_previous(int32_t iterator, uint64_t& primary) {
      return sec_lookup_i256.previous_secondary(iterator, primary);
   }

   /**
    * interface for double secondary
    */
   int32_t db_context_rocksdb::db_idx_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                               const float64_t& secondary) {
      return sec_lookup_double.store(name{scope}, name{table}, payer, id, secondary);
   }

   void db_context_rocksdb::db_idx_double_update(int32_t iterator, account_name payer, const float64_t& secondary) {
      sec_lookup_double.update(iterator, payer, secondary);
   }

   void db_context_rocksdb::db_idx_double_remove(int32_t iterator) {
      sec_lookup_double.remove(iterator);
   }

   int32_t db_context_rocksdb::db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                        const float64_t& secondary, uint64_t& primary) {
      return sec_lookup_double.find_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                      uint64_t primary) {
      return sec_lookup_double.find_primary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                    uint64_t& primary) {
      return sec_lookup_double.lowerbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                    uint64_t& primary) {
      return sec_lookup_double.upperbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table) {
      return sec_lookup_double.end_secondary(name{code}, name{scope}, name{table});
   }

   int32_t db_context_rocksdb::db_idx_double_next(int32_t iterator, uint64_t& primary) {
      return sec_lookup_double.next_secondary(iterator, primary);
   }

   int32_t db_context_rocksdb::db_idx_double_previous(int32_t iterator, uint64_t& primary) {
      return sec_lookup_double.previous_secondary(iterator, primary);
   }

   /**
    * interface for long double secondary
    */
   int32_t db_context_rocksdb::db_idx_long_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                                        const float128_t& secondary) {
      return sec_lookup_long_double.store(name{scope}, name{table}, payer, id, secondary);
   }

   void db_context_rocksdb::db_idx_long_double_update(int32_t iterator, account_name payer, const float128_t& secondary) {
      sec_lookup_long_double.update(iterator, payer, secondary);
   }

   void db_context_rocksdb::db_idx_long_double_remove(int32_t iterator) {
      sec_lookup_long_double.remove(iterator);
   }

   int32_t db_context_rocksdb::db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                                 const float128_t& secondary, uint64_t& primary) {
      return sec_lookup_long_double.find_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table,
                                                               float128_t& secondary, uint64_t primary) {
      return sec_lookup_long_double.find_primary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                                             uint64_t& primary) {
      return sec_lookup_long_double.lowerbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                                             uint64_t& primary) {
      return sec_lookup_long_double.upperbound_secondary(name{code}, name{scope}, name{table}, secondary, primary);
   }

   int32_t db_context_rocksdb::db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table) {
      return sec_lookup_long_double.end_secondary(name{code}, name{scope}, name{table});
   }

   int32_t db_context_rocksdb::db_idx_long_double_next(int32_t iterator, uint64_t& primary) {
      return sec_lookup_long_double.next_secondary(iterator, primary);
   }

   int32_t db_context_rocksdb::db_idx_long_double_previous(int32_t iterator, uint64_t& primary) {
      return sec_lookup_long_double.previous_secondary(iterator, primary);
   }

   // for primary keys in the iterator store, we don't care about secondary key
   db_context_rocksdb::prim_key_iter_type db_context_rocksdb::primary_key_iter(int table_ei, uint64_t key, account_name payer) {
      return prim_key_iter_type { .table_ei = table_ei, .secondary = noop_secondary, .primary = key, .payer = payer};
   }

   void db_context_rocksdb::swap(int iterator, account_name payer) {
      primary_iter_store.swap(iterator, noop_secondary, payer);
   }

   // gets a prefix that allows for only primary key iterators
   prefix_bundle db_context_rocksdb::get_primary_slice_in_primaries(name code, name scope, name table, uint64_t id) {
      bytes primary_key = db_key_value_format::create_primary_key(scope, table, id);
      return { primary_key, end_of_prefix::at_type, code };
   }

   pv_bundle db_context_rocksdb::get_primary_key_value(name code, name scope, name table, uint64_t id) {
      prefix_bundle primary_key = get_primary_slice_in_primaries(code, scope, table, id);
      return { primary_key, current_session.read(primary_key.full_key) };
   }

   void db_context_rocksdb::set_value(const shared_bytes& primary_key, const payer_payload& pp) {
      current_session.write(primary_key, pp.as_payload());
   }

   void db_context_rocksdb::update_db_usage( const account_name& payer, int64_t delta, const storage_usage_trace& trace ) {
      context.update_db_usage(payer, delta, trace);
   }

   int32_t db_context_rocksdb::find_and_store_primary_key(const session_type::iterator& session_iter,
                                                          int32_t table_ei, const shared_bytes& type_prefix,
                                                          int32_t not_found_return, const char* calling_func,
                                                          uint64_t& found_key) {
      // if nothing remains in the database, return the passed in value
      if (session_iter == current_session.end() || !primary_lookup.match_prefix(type_prefix, (*session_iter).first)) {
         return not_found_return;
      }
      EOS_ASSERT( db_key_value_format::get_primary_key((*session_iter).first, type_prefix, found_key), db_rocksdb_invalid_operation_exception,
                  "invariant failure in find_and_store_primary_key, iter store found to update but no primary keys in database");

      const account_name found_payer = payer_payload(*(*session_iter).second).payer;

      return primary_iter_store.add(primary_key_iter(table_ei, found_key, found_payer));
   }

   // returns the exact iterator and the bounding key (type)
   db_context_rocksdb::exact_iterator db_context_rocksdb::get_exact_iterator(
         name code, name scope, name table, uint64_t primary) {
      const auto slice_primary_key = get_primary_slice_in_primaries(code, scope, table, primary);
      auto session_iter = current_session.lower_bound(slice_primary_key.full_key);
      const bool valid = primary_lookup.match(slice_primary_key.full_key, session_iter);
      return { .valid = valid, .itr = std::move(session_iter), .type_prefix = std::move(slice_primary_key.prefix_key) };
   }

   int32_t db_context_rocksdb::find_i64(name code, name scope, name table, uint64_t id, comp comparison) {
      // expanding the "in-play" iterator space to include every key type for that table, to ensure we know if
      // the key is not found, that there is anything in the table at all (and thus can return an end iterator
      // or if an invalid iterator needs to be returned
      prefix_bundle primary_and_prefix_keys { db_key_value_format::create_primary_key(scope, table, id),
                                              end_of_prefix::pre_type, code };
      auto session_iter = current_session.lower_bound(primary_and_prefix_keys.full_key);
      auto is_in_table = [&prefix_key=primary_and_prefix_keys.prefix_key,
                          &primary_lookup=this->primary_lookup](const session_type::iterator& iter) {
         return primary_lookup.match_prefix(prefix_key, iter);
      };
      if (!is_in_table(session_iter)) {
         // if no iterator was found in this table, then there is no table entry, so the table is empty
         return primary_iter_store.invalid_iterator();
      }

      const unique_table t { code, scope, table };
      const auto table_ei = primary_iter_store.cache_table(t);

      const auto& desired_type_prefix =
            db_key_value_format::create_full_key_prefix(primary_and_prefix_keys.full_key, end_of_prefix::at_type);

      std::optional<uint64_t> primary_key;
      if (!primary_lookup.match(primary_and_prefix_keys.full_key, (*session_iter).first)) {
         if (comparison == comp::equals) {
            return table_ei;
         }
         else if (!primary_lookup.match_prefix(desired_type_prefix, (*session_iter).first)) {
            return table_ei;
         }
      }
      else if (comparison == comp::gt) {
         ++session_iter;
         // if we don't have a match, we need to identify if we went past the primary types, and thus are at the end
         EOS_ASSERT( is_in_table(session_iter), db_rocksdb_invalid_operation_exception,
                     "invariant failure in find_i64, primary key found but was not followed by a table key");
         if (!primary_lookup.match_prefix(desired_type_prefix, session_iter)) {
            return table_ei;
         }
      }
      else {
         // since key is exact, and we didn't advance, it is id
         primary_key = id;
      }

      const account_name found_payer = payer_payload(*(*session_iter).second).payer;
      if (!primary_key) {
         uint64_t key = 0;
         const auto valid = db_key_value_format::get_primary_key((*session_iter).first, desired_type_prefix, key);
         EOS_ASSERT( valid, db_rocksdb_invalid_operation_exception,
                     "invariant failure in find_i64, primary key already verified but method indicates it is not a primary key");
         primary_key = key;
      }

      return primary_iter_store.add(primary_key_iter(table_ei, *primary_key, found_payer));
   }

   std::unique_ptr<db_context> create_db_rocksdb_context(apply_context& context, name receiver,
                                                         db_context_rocksdb::session_type& session)
   {
      static_assert(std::is_convertible<db_context_rocksdb *, db_context *>::value, "cannot convert");
      static_assert(std::is_convertible<std::default_delete<db_context_rocksdb>, std::default_delete<db_context> >::value, "cannot convert delete");
      return std::make_unique<db_context_rocksdb>(context, receiver, session);
   }

}}} // namespace eosio::chain::backing_store
