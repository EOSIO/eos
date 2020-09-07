#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>
#include <eosio/chain/backing_store/db_key_value_iter_store.hpp>
#include <eosio/chain/backing_store/chain_kv_payer.hpp>
#include <chain_kv/chain_kv.hpp>
#include <eosio/chain/combined_database.hpp>

namespace eosio { namespace chain { namespace backing_store {
   static constexpr auto payer_size = sizeof(account_name);

   class db_context_rocksdb;

   template<typename SecondaryKey >
   class secondary_lookup
   {
   public:
      secondary_lookup( db_context_rocksdb& p ) : parent(p){}

      int store( name scope, name table, const account_name& payer,
                 uint64_t id, const SecondaryKey& value );
      void remove( int iterator );
      void update( int iterator, account_name payer, const SecondaryKey& secondary );
      int find_secondary( name code, name scope, name table, const SecondaryKey& secondary, uint64_t& primary );
      int lowerbound_secondary( name code, name scope, name table, SecondaryKey& secondary, uint64_t& primary );
      int upperbound_secondary( name code, name scope, name table, SecondaryKey& secondary, uint64_t& primary );
      int end_secondary( name code, name scope, name table );
      int next_secondary( int iterator, uint64_t& primary );
      int previous_secondary( int iterator, uint64_t& primary );
      int find_primary( name code, name scope, name table, SecondaryKey& secondary, uint64_t primary );
      int lowerbound_primary( name code, name scope, name table, uint64_t primary );
      int upperbound_primary( name code, name scope, name table, uint64_t primary );
      int next_primary( int iterator, uint64_t& primary );
      int previous_primary( int iterator, uint64_t& primary );
      void get( int iterator, uint64_t& primary, SecondaryKey& secondary );

   private:
      db_context_rocksdb&                   parent;
      db_key_value_iter_store<SecondaryKey> primary_iter_store;
   };

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
      void add_table_if_needed(name scope, name table, account_name payer);
      void remove_table_if_empty(name scope, name table, account_name payer);

      // for primary keys in the iterator store, we don't care about secondary key
      static prim_key_iter_type primary_key_iter(int table_ei, uint64_t key, account_name payer);
      void swap(int iterator, account_name payer);

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

      // gets a prefix that allows for only primary key iterators
      static prefix_bundle get_primary_slice_in_primaries(name scope, name table, uint64_t id);
      // gets a prefix that allows for a specific primary key, but will allow for all iterators in the table
      static prefix_bundle get_primary_slice_in_table(name scope, name table, uint64_t id);
      static key_bundle get_slice(name scope, name table);
      static key_bundle get_table_end_slice(name scope, name table);
      pv_bundle get_primary_key_value(name code, name scope, name table, uint64_t id);
      void set_value(const rocksdb::Slice& primary_key, const char payer_buf[payer_size], const char* value , size_t value_size);
      void update_db_usage( const account_name& payer, int64_t delta, const storage_usage_trace& trace );
      int32_t find_and_store_primary_key(const b1::chain_kv::view::iterator& chain_kv_iter, int32_t table_ei, name scope,
                                         name table, int32_t not_found_return, const char* calling_func, uint64_t& found_key);
      b1::chain_kv::view::iterator get_exact_iterator(name code, name scope, name table, uint64_t primary);

      enum class comp { equals, gte, gt};
      int32_t find_i64(name code, name scope, name table, uint64_t id, comp comparison);

      apply_context&                    context;
      const name                        receiver;
      b1::chain_kv::undo_stack&         undo_stack;
      b1::chain_kv::write_session       write_session;
      b1::chain_kv::view                view;
      db_key_value_iter_store<uint64_t> primary_iter_store;
      static constexpr uint64_t         noop_secondary = 0x0;
      static constexpr int64_t          overhead = config::billable_size_v<key_value_object>;
      static constexpr int64_t          table_overhead = config::billable_size_v<table_id_object>;
   }; // db_context_rocksdb

   db_context_rocksdb::db_context_rocksdb(apply_context& context, name receiver, b1::chain_kv::database& database,
                                          b1::chain_kv::undo_stack& undo_stack)
   : context{ context }, receiver{ receiver }, undo_stack{ undo_stack }, write_session{ database },
     view{ write_session, make_prefix() } {}

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
      return rocksdb_contract_db_prefix;
   }

   int32_t db_context_rocksdb::db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char* value , size_t value_size) {
      const name scope_name{scope};
      const name table_name{table};
      const auto old_key_value = get_primary_key_value(receiver, scope_name, table_name, id);

      EOS_ASSERT( !old_key_value.value, db_rocksdb_invalid_operation_exception, "db_store_i64 called with pre-existing key");

      char payer_buf[payer_size];
      memcpy(payer_buf, &payer, payer_size);
      set_value(old_key_value.key, payer_buf, value, value_size);

      const int64_t billable_size = (int64_t)(value_size + overhead);

      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = db_context::table_event(receiver, scope_name, table_name, name(id));
      }

      update_db_usage( payer, billable_size, db_context::row_add_trace(context.get_action_id(), event_id.c_str()) );

      if (auto dm_logger = context.control.get_deep_mind_logger()) {
         db_context::write_row_insert(*dm_logger, context.get_action_id(), receiver, scope_name, table_name, payer,
               name(id), value, value_size);
      }

      add_table_if_needed(scope_name, table_name, payer);

      const unique_table t { receiver, scope_name, table_name };
      const auto table_ei = primary_iter_store.cache_table(t);
      return primary_iter_store.add(primary_key_iter(table_ei, id, payer));
   }

   void db_context_rocksdb::add_table_if_needed(name scope, name table, account_name payer) {
      const bytes table_key = db_key_value_format::create_table_key(scope, table);
      const rocksdb::Slice slice_table_key{table_key.data(), table_key.size()};
      if (!view.get(receiver.to_uint64_t(), slice_table_key)) {
         std::string event_id;
         if (context.control.get_deep_mind_logger() != nullptr) {
            event_id = db_context::table_event(receiver, scope, table);
         }

         update_db_usage(payer, table_overhead, db_context::add_table_trace(context.get_action_id(), event_id.c_str()));

         char payer_buf[payer_size];
         memcpy(payer_buf, &payer, payer_size);
         view.set(receiver.to_uint64_t(), slice_table_key, {payer_buf, payer_size});

         if (auto dm_logger = context.control.get_deep_mind_logger()) {
            db_context::write_insert_table(*dm_logger, context.get_action_id(), receiver, scope, table, payer);
         }
      }
   }

   void db_context_rocksdb::remove_table_if_empty(name scope, name table, account_name payer) {
      // look for any other entries in the table
      const key_bundle slice_prefix_key = get_slice(scope, table);
      // since this prefix key is just scope and table, it will include all primary, secondary, and table keys
      b1::chain_kv::view::iterator chain_kv_iter{ view, receiver.to_uint64_t(), slice_prefix_key.key };
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

      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = db_context::table_event(receiver, scope, table);
      }

      update_db_usage(payer, - table_overhead, db_context::rem_table_trace(context.get_action_id(), event_id.c_str()) );

      if (auto dm_logger = context.control.get_deep_mind_logger()) {
         db_context::write_remove_table(*dm_logger, context.get_action_id(), receiver, scope, table, payer);
      }

      view.erase(receiver.to_uint64_t(), first_kv->key);
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

      char payer_buf[payer_size];
      memcpy(payer_buf, &payer, payer_size);
      set_value(old_key_value.key, payer_buf, value, value_size);

      const auto old_value_actual_size = backing_store::actual_value_size(old_key_value.value->size());

      std::string event_id;
      if (context.control.get_deep_mind_logger() != nullptr) {
         event_id = db_context::table_event(table_store.contract, table_store.scope, table_store.table, name(key_store.primary));
      }

      const int64_t old_size = (int64_t)(old_value_actual_size + overhead);
      const int64_t new_size = (int64_t)(value_size + overhead);
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

      update_db_usage( old_payer,  -((int64_t)old_key_value.value->size() + overhead), db_context::row_rem_trace(context.get_action_id(), event_id.c_str()) );

      if (auto dm_logger = context.control.get_deep_mind_logger()) {
         db_context::write_row_remove(*dm_logger, context.get_action_id(), table_store.contract, table_store.scope,
         table_store.table, old_payer, name(key_store.primary), old_key_value.value->data(), old_key_value.value->size());
      }

      view.erase(receiver.to_uint64_t(), old_key_value.key);
      remove_table_if_empty(table_store.scope, table_store.table, old_payer);

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
      const size_t copy_size = std::min<size_t>(value_size, actual_size);
      memcpy( value, actual_value, copy_size );
      return copy_size;
   }

   int32_t db_context_rocksdb::db_next_i64(int32_t itr, uint64_t& primary) {
      if (itr < -1) return -1; // cannot increment past end iterator of table

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
      if( itr < -1 ) // is end iterator
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
      const auto table_key = get_table_end_slice(name{scope}, name{table});
      std::shared_ptr<const bytes> value = view.get(code, table_key.key);
      if (!value) {
         return primary_iter_store.invalid_iterator();
      }

      const unique_table t { name{code}, name{scope}, name{table} };
      const auto table_ei = primary_iter_store.cache_table(t);
      return table_ei;
   }

   /**
    * interface for uint64_t secondary
    */
   int32_t db_context_rocksdb::db_idx64_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                          const uint64_t& secondary) {
      return -1;
   }

   void db_context_rocksdb::db_idx64_update(int32_t iterator, account_name payer, const uint64_t& secondary) {
   }

   void db_context_rocksdb::db_idx64_remove(int32_t iterator) {
   }

   int32_t db_context_rocksdb::db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint64_t& secondary,
                                   uint64_t& primary) {
      return -1;
   }

   int32_t db_context_rocksdb::db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                 uint64_t primary) {
      return -1;
   }

   int32_t db_context_rocksdb::db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                               uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                               uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx64_end(uint64_t code, uint64_t scope, uint64_t table) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx64_next(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx64_previous(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   /**
    * interface for uint128_t secondary
    */
   int32_t db_context_rocksdb::db_idx128_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                           const uint128_t& secondary) {
   return -1;
   }

   void db_context_rocksdb::db_idx128_update(int32_t iterator, account_name payer, const uint128_t& secondary) {
   }

   void db_context_rocksdb::db_idx128_remove(int32_t iterator) {
   }

   int32_t db_context_rocksdb::db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t& secondary,
                                    uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                  uint64_t primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx128_end(uint64_t code, uint64_t scope, uint64_t table) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx128_next(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx128_previous(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   /**
    * interface for 256-bit interger secondary
    */
   int32_t db_context_rocksdb::db_idx256_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                           const uint128_t* data) {
   return -1;
   }

   void db_context_rocksdb::db_idx256_update(int32_t iterator, account_name payer, const uint128_t* data) {
   }

   void db_context_rocksdb::db_idx256_remove(int32_t iterator) {
   }

   int32_t db_context_rocksdb::db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* data,
                                    uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                  uint64_t primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx256_next(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx256_previous(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   /**
    * interface for double secondary
    */
   int32_t db_context_rocksdb::db_idx_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                               const float64_t& secondary) {
   return -1;
   }

   void db_context_rocksdb::db_idx_double_update(int32_t iterator, account_name payer, const float64_t& secondary) {
   }

   void db_context_rocksdb::db_idx_double_remove(int32_t iterator) {
   }

   int32_t db_context_rocksdb::db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                        const float64_t& secondary, uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                      uint64_t primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                    uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                    uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_double_next(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_double_previous(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   /**
    * interface for long double secondary
    */
   int32_t db_context_rocksdb::db_idx_long_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                                        const float128_t& secondary) {
   return -1;
   }

   void db_context_rocksdb::db_idx_long_double_update(int32_t iterator, account_name payer, const float128_t& secondary) {
   }

   void db_context_rocksdb::db_idx_long_double_remove(int32_t iterator) {
   }

   int32_t db_context_rocksdb::db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                             const float128_t& secondary, uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table,
                                           float128_t& secondary, uint64_t primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                         uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float128_t& secondary,
                                         uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_long_double_next(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   int32_t db_context_rocksdb::db_idx_long_double_previous(int32_t iterator, uint64_t& primary) {
   return -1;
   }

   // for primary keys in the iterator store, we don't care about secondary key
   db_context_rocksdb::prim_key_iter_type db_context_rocksdb::primary_key_iter(int table_ei, uint64_t key, account_name payer) {
      return prim_key_iter_type { .table_ei = table_ei, .secondary = noop_secondary, .primary = key, .payer = payer};
   }

   void db_context_rocksdb::swap(int iterator, account_name payer) {
      primary_iter_store.swap(iterator, noop_secondary, payer);
   }

   // gets a prefix that allows for only primary key iterators
   db_context_rocksdb::prefix_bundle db_context_rocksdb::get_primary_slice_in_primaries(name scope, name table, uint64_t id) {
      const bytes primary_key = db_key_value_format::create_primary_key(scope, table, id);
      const rocksdb::Slice prefix = db_key_value_format::prefix_type_slice(primary_key);
      const rocksdb::Slice key = {primary_key.data(), primary_key.size()};
      return { .composite_key = std::move(primary_key), .key = std::move(key), .prefix = std::move(prefix) };
   }

   // gets a prefix that allows for a specific primary key, but will allow for all iterators in the table
   db_context_rocksdb::prefix_bundle db_context_rocksdb::get_primary_slice_in_table(name scope, name table, uint64_t id) {
      const bytes primary_key = db_key_value_format::create_primary_key(scope, table, id);
      const rocksdb::Slice prefix = db_key_value_format::prefix_type_slice(primary_key);
      const rocksdb::Slice key = {primary_key.data(), primary_key.size()};
      return { .composite_key = std::move(primary_key), .key = std::move(key), .prefix = std::move(prefix) };
   }

   db_context_rocksdb::key_bundle db_context_rocksdb::get_slice(name scope, name table) {
      const bytes prefix_key = db_key_value_format::create_prefix_key(scope, table);
      const rocksdb::Slice key {prefix_key.data(), prefix_key.size()};
      return { .composite_key = std::move(prefix_key), .key = std::move(key) };
   }

   db_context_rocksdb::key_bundle db_context_rocksdb::get_table_end_slice(name scope, name table) {
      const bytes table_key = db_key_value_format::create_table_key(scope, table);
      const rocksdb::Slice key = {table_key.data(), table_key.size()};
      return { .composite_key = std::move(table_key), .key = std::move(key) };
   }

   db_context_rocksdb::pv_bundle db_context_rocksdb::get_primary_key_value(name code, name scope, name table, uint64_t id) {
      prefix_bundle primary_key = get_primary_slice_in_primaries(scope, table, id);
      std::shared_ptr<const bytes> value = view.get(code.to_uint64_t(), primary_key.key);
      EOS_ASSERT( !value || value->size() > payer_in_value_size, db_rocksdb_invalid_operation_exception,
      "invariant failure value found in database, but not big enough to contain at least a payer");
      return { std::move(primary_key), std::move(value) };
   }

   void db_context_rocksdb::set_value(const rocksdb::Slice& primary_key, const char payer_buf[payer_size], const char* value , size_t value_size) {
      bytes total_value;
      const uint32_t total_value_size = payer_size + value_size;
      total_value.reserve(total_value_size);

      total_value.insert(total_value.end(), payer_buf, payer_buf + payer_size);

      total_value.insert(total_value.end(), value, value + value_size);
      view.set(receiver.to_uint64_t(), primary_key, {total_value.data(), total_value_size});
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
      auto is_same_type = [](const rocksdb::Slice& desired, const rocksdb::Slice& found) {
         const auto found_key_type_prefix = db_key_value_format::prefix_type_slice(found);
         return strncmp(found_key_type_prefix.data(), desired.data(), found_key_type_prefix.size()) == 0;
      };

      if (found_kv->key != slice_primary_key.key) {
         if (comparison == comp::equals) {
            return table_ei;
         }
         else if (is_same_type(slice_primary_key.key, found_kv->key)) {
            return table_ei;
         }
      }
      else if (comparison == comp::gt) {
         ++chain_kv_iter;
         found_kv = chain_kv_iter.get_kv();
         EOS_ASSERT( found_kv, db_rocksdb_invalid_operation_exception,
         "invariant failure in find_i64, primary key found but was not followed by a table key");
         if (is_same_type(slice_primary_key.key, found_kv->key)) {
            return table_ei;
         }
      }
      backing_store::actual_value_size(found_kv->value.size()); // validate that the value at least has a payer
      const account_name found_payer = backing_store::get_payer(found_kv->value.data());

      return primary_iter_store.add(primary_key_iter(table_ei, id, found_payer));
   }

   std::unique_ptr<db_context> create_db_rocksdb_context(apply_context& context, name receiver,
                                                         b1::chain_kv::database& database, b1::chain_kv::undo_stack& undo_stack)
   {
      static_assert(std::is_convertible<db_context_rocksdb *, db_context *>::value, "cannot convert");
      static_assert(std::is_convertible<std::default_delete<db_context_rocksdb>, std::default_delete<db_context> >::value, "cannot convert delete");
      return std::make_unique<db_context_rocksdb>(context, receiver, database, undo_stack);
   }

}}} // namespace eosio::chain::backing_store
