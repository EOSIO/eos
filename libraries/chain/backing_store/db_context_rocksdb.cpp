#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/db_key_value_iter_store.hpp>
#include <eosio/chain/backing_store/db_key_value_any_lookup.hpp>
#include <eosio/chain/backing_store/db_key_value_sec_lookup.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>
#include <chain_kv/chain_kv.hpp>
#include <eosio/chain/combined_database.hpp>

namespace eosio { namespace chain { namespace backing_store {

   class db_context_rocksdb : public db_context {
   public:
      using prim_key_iter_type = secondary_key<uint64_t>;
      db_context_rocksdb(apply_context& context, name receiver, b1::chain_kv::database& database,
                         b1::chain_kv::undo_stack& undo_stack);

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

      std::vector<char> make_prefix();

      // for primary keys in the iterator store, we don't care about secondary key
      static prim_key_iter_type primary_key_iter(int table_ei, uint64_t key, account_name payer);
      void swap(int iterator, account_name payer);

      // gets a prefix that allows for only primary key iterators
      static prefix_bundle get_primary_slice_in_primaries(name scope, name table, uint64_t id);
      // gets a prefix that allows for a specific primary key, but will allow for all iterators in the table
      static prefix_bundle get_primary_slice_in_table(name scope, name table, uint64_t id);
      pv_bundle get_primary_key_value(name code, name scope, name table, uint64_t id);
      void set_value(const rocksdb::Slice& primary_key, const payer_payload& pp);
      void update_db_usage( const account_name& payer, int64_t delta, const storage_usage_trace& trace );
      int32_t find_and_store_primary_key(const b1::chain_kv::view::iterator& chain_kv_iter, int32_t table_ei, name scope,
                                         name table, int32_t not_found_return, const char* calling_func, uint64_t& found_key);
      b1::chain_kv::view::iterator get_exact_iterator(name code, name scope, name table, uint64_t primary);

      enum class comp { equals, gte, gt};
      int32_t find_i64(name code, name scope, name table, uint64_t id, comp comparison);

      using uint128_t = eosio::chain::uint128_t;
      using key256_t = eosio::chain::key256_t;
      b1::chain_kv::undo_stack&            undo_stack;
      b1::chain_kv::write_session          write_session;
      b1::chain_kv::view                   view;
      db_key_value_iter_store<uint64_t>    primary_iter_store;
      db_key_value_any_lookup              primary_lookup;
      db_key_value_sec_lookup<uint64_t>    sec_lookup_i64;
      db_key_value_sec_lookup<uint128_t>   sec_lookup_i128;
      db_key_value_sec_lookup<key256_t>    sec_lookup_i256;
      db_key_value_sec_lookup<float64_t>   sec_lookup_double;
      db_key_value_sec_lookup<float128_t>  sec_lookup_long_double;
      static constexpr uint64_t            noop_secondary = 0x0;
   }; // db_context_rocksdb

   db_context_rocksdb::db_context_rocksdb(apply_context& context, name receiver, b1::chain_kv::database& database,
                                          b1::chain_kv::undo_stack& undo_stack)
   : db_context( context, receiver ), undo_stack{ undo_stack }, write_session{ database },
     view{ write_session, make_prefix() }, primary_lookup(*this, view), sec_lookup_i64(*this, view),
     sec_lookup_i128(*this, view), sec_lookup_i256(*this, view), sec_lookup_double(*this, view),
     sec_lookup_long_double(*this, view) {}

   db_context_rocksdb::~db_context_rocksdb() {
      try {
         try {
            write_session.write_changes(undo_stack);
         }
         FC_LOG_AND_RETHROW()
      }
      CATCH_AND_EXIT_DB_FAILURE()
   }

   std::vector<char> db_context_rocksdb::make_prefix() {
      return make_rocksdb_contract_db_prefix();
   }

   int32_t db_context_rocksdb::db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char* value , size_t value_size) {
      EOS_ASSERT( payer != account_name(), invalid_table_payer, "must specify a valid account to pay for new record" );
      const name scope_name{scope};
      const name table_name{table};
      const auto old_key_value = get_primary_key_value(receiver, scope_name, table_name, id);

      EOS_ASSERT( !old_key_value.value, db_rocksdb_invalid_operation_exception, "db_store_i64 called with pre-existing key");

      const payer_payload pp{payer, value, value_size};
      set_value(old_key_value.key, pp);

      const int64_t billable_size = (int64_t)(value_size + db_key_value_any_lookup::overhead);

      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = db_context::table_event(receiver, scope_name, table_name, name(id));
      }

      update_db_usage( payer, billable_size, db_context::row_add_trace(context.get_action_id(), event_id.c_str()) );

      if (auto dm_logger = context.control.get_deep_mind_logger()) {
         db_context::write_row_insert(*dm_logger, context.get_action_id(), receiver, scope_name, table_name, payer,
               name(id), value, value_size);
      }

#warning Should move this up to beginning to maintain same order
      primary_lookup.add_table_if_needed(scope_name, table_name, payer);

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
      if (payer == name{}) {
         payer = old_payer;
      }

      const payer_payload pp{payer, value, value_size};
      set_value(old_key_value.key, pp);

      const auto old_value_actual_size = backing_store::actual_value_size(old_key_value.value->size());

      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = db_context::table_event(table_store.contract, table_store.scope, table_store.table, name(key_store.primary));
      }

      const int64_t old_size = (int64_t)(old_value_actual_size + db_key_value_any_lookup::overhead);
      const int64_t new_size = (int64_t)(value_size + db_key_value_any_lookup::overhead);
      if( old_payer != payer ) {
         // refund the existing payer
         update_db_usage( old_payer, -(old_size), db_context::row_update_rem_trace(context.get_action_id(), event_id.c_str()) );
         // charge the new payer
         update_db_usage( payer,  (new_size), db_context::row_update_add_trace(context.get_action_id(), event_id.c_str()) );

         // swap the payer in the iterator store
         swap(itr, payer);
      } else if(old_size != new_size) {
         // charge/refund the existing payer the difference
         update_db_usage( old_payer, new_size - old_size, db_context::row_update_trace(context.get_action_id(), event_id.c_str()) );
      }

      if (auto dm_logger = context.control.get_deep_mind_logger()) {
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
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = db_context::table_event(table_store.contract, table_store.scope, table_store.table, name(key_store.primary));
      }

      payer_payload pp(*old_key_value.value);
      update_db_usage( old_payer,  -((int64_t)pp.value_size + db_key_value_any_lookup::overhead), db_context::row_rem_trace(context.get_action_id(), event_id.c_str()) );

      if (auto dm_logger = context.control.get_deep_mind_logger()) {
         db_context::write_row_remove(*dm_logger, context.get_action_id(), table_store.contract, table_store.scope,
                                      table_store.table, old_payer, name(key_store.primary), old_key_value.value->data(),
                                      old_key_value.value->size());
      }

      view.erase(receiver.to_uint64_t(), old_key_value.key);
      primary_lookup.remove_table_if_empty(table_store.scope, table_store.table);

      primary_iter_store.remove(itr); // don't use key_store anymore
   }

   int32_t db_context_rocksdb::db_get_i64(int32_t itr, char* value , size_t value_size) {
      const auto& key_store = primary_iter_store.get(itr);
      const auto& table_store = primary_iter_store.get_table(key_store);
      const auto old_key_value = get_primary_key_value(table_store.contract, table_store.scope, table_store.table, key_store.primary);

      EOS_ASSERT( old_key_value.value, db_rocksdb_invalid_operation_exception,
                  "invariant failure in db_get_i64, iter store found to update but nothing in database");
      const char* const actual_value = backing_store::actual_value_start(old_key_value.value->data());
      const size_t actual_size = backing_store::actual_value_size(old_key_value.value->size());
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
      b1::chain_kv::view::iterator chain_kv_iter =
            get_exact_iterator(table_store.contract, table_store.scope, table_store.table, key_store.primary);
      ++chain_kv_iter;
      return find_and_store_primary_key(chain_kv_iter, key_store.table_ei, table_store.scope, table_store.table,
                                        key_store.table_ei, __func__, primary);
   }

   int32_t db_context_rocksdb::db_previous_i64(int32_t itr, uint64_t& primary) {
      int32_t table_ei = primary_iter_store.invalid_iterator();
      if( itr < primary_iter_store.invalid_iterator() ) // is end iterator
      {
         const backing_store::unique_table* table_store = primary_iter_store.find_table_by_end_iterator(itr);
         EOS_ASSERT( table_store, invalid_table_iterator, "not a valid end iterator" );
         const auto primary_bounded_key = get_primary_slice_in_primaries(table_store->scope, table_store->table, 0x0);
         b1::chain_kv::view::iterator chain_kv_iter { view, receiver.to_uint64_t(), primary_bounded_key.prefix };
         chain_kv_iter.move_to_end();
         --chain_kv_iter; // move back to the last primary key
         return find_and_store_primary_key(chain_kv_iter, primary_iter_store.get_end_iterator_by_table(*table_store),
                                           table_store->scope, table_store->table, primary_iter_store.invalid_iterator(),
                                           __func__, primary);
      }

      const auto& key_store = primary_iter_store.get(itr);
      const backing_store::unique_table& table_store = primary_iter_store.get_table(key_store);
      const auto slice_primary_key = get_primary_slice_in_primaries(table_store.scope, table_store.table, key_store.primary);
      b1::chain_kv::view::iterator chain_kv_iter = get_exact_iterator(table_store.contract, table_store.scope, table_store.table, key_store.primary);
      --chain_kv_iter;
      return find_and_store_primary_key(chain_kv_iter, key_store.table_ei, table_store.scope, table_store.table,
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
   prefix_bundle db_context_rocksdb::get_primary_slice_in_primaries(name scope, name table, uint64_t id) {
      bytes primary_key = db_key_value_format::create_primary_key(scope, table, id);
      rocksdb::Slice prefix = db_key_value_format::prefix_type_slice(primary_key);
      rocksdb::Slice key = {primary_key.data(), primary_key.size()};
      prefix_bundle bundle { .composite_key = std::move(primary_key), .key = std::move(key), .prefix = std::move(prefix) };
      return bundle;
   }

   // gets a prefix that allows for a specific primary key, but will allow for all iterators in the table
   prefix_bundle db_context_rocksdb::get_primary_slice_in_table(name scope, name table, uint64_t id) {
      bytes primary_key = db_key_value_format::create_primary_key(scope, table, id);
      rocksdb::Slice prefix = db_key_value_format::prefix_slice(primary_key);
      rocksdb::Slice key = {primary_key.data(), primary_key.size()};
      return { .composite_key = std::move(primary_key), .key = std::move(key), .prefix = std::move(prefix) };
   }

   pv_bundle db_context_rocksdb::get_primary_key_value(name code, name scope, name table, uint64_t id) {
      prefix_bundle primary_key = get_primary_slice_in_primaries(scope, table, id);
      std::shared_ptr<const bytes> value = view.get(code.to_uint64_t(), primary_key.key);
      EOS_ASSERT( !value || value->size() > payer_in_value_size, db_rocksdb_invalid_operation_exception,
                  "invariant failure value found in database, but not big enough to contain at least a payer");
      return { std::move(primary_key), std::move(value) };
   }

   void db_context_rocksdb::set_value(const rocksdb::Slice& primary_key, const payer_payload& pp) {
      const bytes total_value = pp.as_payload();
      view.set(receiver.to_uint64_t(), primary_key, {total_value.data(), total_value.size()});
   }

   void db_context_rocksdb::update_db_usage( const account_name& payer, int64_t delta, const storage_usage_trace& trace ) {
      context.update_db_usage(payer, delta, trace);
   }

   int32_t db_context_rocksdb::find_and_store_primary_key(const b1::chain_kv::view::iterator& chain_kv_iter,
                                                          int32_t table_ei, name scope, name table,
                                                          int32_t not_found_return, const char* calling_func,
                                                          uint64_t& found_key) {
      const auto found_kv = chain_kv_iter.get_kv();
      // if nothing remains in the database, return the table end iterator
      if (!found_kv) {
         return not_found_return;
      }

      const b1::chain_kv::bytes composite_key(found_kv->key.data(), found_kv->key.data() + found_kv->key.size());
      name found_scope;
      name found_table;
      if (!backing_store::db_key_value_format::get_primary_key(composite_key, found_scope, found_table, found_key)) {
         // since this is not a primary key entry, return the table end iterator
         return not_found_return;
      }

      EOS_ASSERT( found_scope == scope && found_table == table, db_rocksdb_invalid_operation_exception,
                  "invariant failure in ${desc}, progressed from primary/secondary key for one table to a "
                  "primary/secondary key for another table, there should at least be an entry for a table key "
                  "between them", ("desc", calling_func));
      backing_store::actual_value_size(found_kv->value.size()); // validate that the value at least has a payer
      const account_name found_payer = backing_store::get_payer(found_kv->value.data());

      return primary_iter_store.add(primary_key_iter(table_ei, found_key, found_payer));
   }

   b1::chain_kv::view::iterator db_context_rocksdb::get_exact_iterator(name code, name scope, name table, uint64_t primary) {
      const auto slice_primary_key = get_primary_slice_in_primaries(scope, table, primary);
      b1::chain_kv::view::iterator chain_kv_iter{view, code.to_uint64_t(), slice_primary_key.prefix};
      chain_kv_iter.lower_bound(slice_primary_key.composite_key);
      const auto kv = chain_kv_iter.get_kv();
      EOS_ASSERT( kv, db_rocksdb_invalid_operation_exception,
      "invariant failure in db_get_i64, iter store found to update but no primary keys in database");
      EOS_ASSERT( kv->key == slice_primary_key.key, db_rocksdb_invalid_operation_exception,
      "invariant failure in db_get_i64, iter store found to update but it does not exist in the database");
      return chain_kv_iter;
   }

   int32_t db_context_rocksdb::find_i64(name code, name scope, name table, uint64_t id, comp comparison) {
      // expanding the "in-play" iterator space to include every key type for that table, to ensure we know if
      // the key is not found, that there is anything in the table at all (and thus can return an end iterator
      // or if an invalid iterator needs to be returned
      const auto slice_primary_key = get_primary_slice_in_table(scope, table, id);
      b1::chain_kv::view::iterator chain_kv_iter{view, code.to_uint64_t(), slice_primary_key.prefix};
      chain_kv_iter.lower_bound(slice_primary_key.composite_key);

      auto found_kv = chain_kv_iter.get_kv();
      if (!found_kv) {
         // if no iterator was found, then there is no table entry, so the table is empty
         return primary_iter_store.invalid_iterator();
      }

      const unique_table t { code, scope, table };
      const auto table_ei = primary_iter_store.cache_table(t);

      // if we don't have a match, we need to identify if we went past the primary types, and thus are at the end
      auto is_prefix_match = [](const rocksdb::Slice& desired, const rocksdb::Slice& found) {
         return memcmp(desired.data(), found.data(), desired.size()) == 0;
      };

      const auto desired_type_prefix = db_key_value_format::prefix_type_slice(slice_primary_key.key);
      std::optional<uint64_t> primary_key;
      if (found_kv->key != slice_primary_key.key) {
         if (comparison == comp::equals) {
            return table_ei;
         }
         else if (!is_prefix_match(desired_type_prefix, found_kv->key)) {
            return table_ei;
         }
      }
      else if (comparison == comp::gt) {
         ++chain_kv_iter;
         found_kv = chain_kv_iter.get_kv();
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
                     "invariant failure in find_i64, primary key found but was not followed by a table key");
         if (!is_prefix_match(desired_type_prefix, found_kv->key)) {
            return table_ei;
         }
      }
      else {
         // since key is exact, and we didn't advance, it is id
         primary_key = id;
      }

      const account_name found_payer = payer_payload(found_kv->value).payer;
      if (!primary_key) {
         uint64_t key = 0;
         const auto valid = db_key_value_format::get_primary_key(found_kv->key, desired_type_prefix, key);
         EOS_ASSERT( valid, db_rocksdb_invalid_operation_exception,
                     "invariant failure in find_i64, primary key found but was not followed by a table key");
         primary_key = key;
      }

      return primary_iter_store.add(primary_key_iter(table_ei, *primary_key, found_payer));
   }

   std::unique_ptr<db_context> create_db_rocksdb_context(apply_context& context, name receiver,
                                                         b1::chain_kv::database& database, b1::chain_kv::undo_stack& undo_stack)
   {
      static_assert(std::is_convertible<db_context_rocksdb *, db_context *>::value, "cannot convert");
      static_assert(std::is_convertible<std::default_delete<db_context_rocksdb>, std::default_delete<db_context> >::value, "cannot convert delete");
      return std::make_unique<db_context_rocksdb>(context, receiver, database, undo_stack);
   }

}}} // namespace eosio::chain::backing_store
