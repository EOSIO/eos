#pragma once

#include <eosio/chain/name.hpp>
#include <memory>
#include <stdint.h>

namespace chainbase {
   class database;
}

namespace eosio {
   namespace session {
      struct rocksdb_t;
      template<typename Parent>
      class session;

      template <typename... T>
      class session_variant;
   }
namespace chain {

      class apply_context;

namespace backing_store {
      struct db_context {
         db_context(apply_context& c, const name& recv) : context(c), receiver(recv) {}

         virtual ~db_context() {}

         virtual int32_t db_store_i64(uint64_t scope, uint64_t table, account_name payer, uint64_t id, const char* buffer , size_t buffer_size) = 0;

         virtual void db_update_i64(int32_t itr, account_name payer, const char* buffer , size_t buffer_size) = 0;

         virtual void db_remove_i64(int32_t itr) = 0;

         virtual int32_t db_get_i64(int32_t itr, char* buffer , size_t buffer_size) = 0;

         virtual int32_t db_next_i64(int32_t itr, uint64_t& primary) = 0;

         virtual int32_t db_previous_i64(int32_t itr, uint64_t& primary) = 0;

         virtual int32_t db_find_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) = 0;

         virtual int32_t db_lowerbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) = 0;

         virtual int32_t db_upperbound_i64(uint64_t code, uint64_t scope, uint64_t table, uint64_t id) = 0;

         virtual int32_t db_end_i64(uint64_t code, uint64_t scope, uint64_t table) = 0;

         /**
          * interface for uint64_t secondary
          */
         virtual int32_t db_idx64_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                const uint64_t& secondary) = 0;

         virtual void db_idx64_update(int32_t iterator, account_name payer, const uint64_t& secondary) = 0;

         virtual void db_idx64_remove(int32_t iterator) = 0;

         virtual int32_t db_idx64_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                 const uint64_t& secondary, uint64_t& primary) = 0;

         virtual int32_t db_idx64_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                               uint64_t primary) = 0;

         virtual int32_t db_idx64_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                             uint64_t& primary) = 0;

         virtual int32_t db_idx64_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint64_t& secondary,
                                             uint64_t& primary) = 0;

         virtual int32_t db_idx64_end(uint64_t code, uint64_t scope, uint64_t table) = 0;

         virtual int32_t db_idx64_next(int32_t iterator, uint64_t& primary) = 0;

         virtual int32_t db_idx64_previous(int32_t iterator, uint64_t& primary) = 0;

         /**
          * interface for uint128_t secondary
          */
         virtual int32_t db_idx128_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                         const uint128_t& secondary) = 0;

         virtual void db_idx128_update(int32_t iterator, account_name payer, const uint128_t& secondary) = 0;

         virtual void db_idx128_remove(int32_t iterator) = 0;

         virtual int32_t db_idx128_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                  const uint128_t& secondary, uint64_t& primary) = 0;

         virtual int32_t db_idx128_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                                uint64_t primary) = 0;

         virtual int32_t db_idx128_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                              uint64_t& primary) = 0;

         virtual int32_t db_idx128_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t& secondary,
                                              uint64_t& primary) = 0;

         virtual int32_t db_idx128_end(uint64_t code, uint64_t scope, uint64_t table) = 0;

         virtual int32_t db_idx128_next(int32_t iterator, uint64_t& primary) = 0;

         virtual int32_t db_idx128_previous(int32_t iterator, uint64_t& primary) = 0;

         /**
          * interface for 256-bit interger secondary
          */
         virtual int32_t db_idx256_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                         const uint128_t* data) = 0;

         virtual void db_idx256_update(int32_t iterator, account_name payer, const uint128_t* data) = 0;

         virtual void db_idx256_remove(int32_t iterator) = 0;

         virtual int32_t db_idx256_find_secondary(uint64_t code, uint64_t scope, uint64_t table, const uint128_t* data,
                                                  uint64_t& primary) = 0;

         virtual int32_t db_idx256_find_primary(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                                uint64_t primary) = 0;

         virtual int32_t db_idx256_lowerbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                              uint64_t& primary) = 0;

         virtual int32_t db_idx256_upperbound(uint64_t code, uint64_t scope, uint64_t table, uint128_t* data,
                                              uint64_t& primary) = 0;

         virtual int32_t db_idx256_end(uint64_t code, uint64_t scope, uint64_t table) = 0;

         virtual int32_t db_idx256_next(int32_t iterator, uint64_t& primary) = 0;

         virtual int32_t db_idx256_previous(int32_t iterator, uint64_t& primary) = 0;

         /**
          * interface for double secondary
          */
         virtual int32_t db_idx_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                             const float64_t& secondary) = 0;

         virtual void db_idx_double_update(int32_t iterator, account_name payer, const float64_t& secondary) = 0;

         virtual void db_idx_double_remove(int32_t iterator) = 0;

         virtual int32_t db_idx_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                      const float64_t& secondary, uint64_t& primary) = 0;

         virtual int32_t db_idx_double_find_primary(uint64_t code, uint64_t scope, uint64_t table,
                                                    float64_t& secondary, uint64_t primary) = 0;

         virtual int32_t db_idx_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                                  uint64_t& primary) = 0;

         virtual int32_t db_idx_double_upperbound(uint64_t code, uint64_t scope, uint64_t table, float64_t& secondary,
                                                  uint64_t& primary) = 0;

         virtual int32_t db_idx_double_end(uint64_t code, uint64_t scope, uint64_t table) = 0;

         virtual int32_t db_idx_double_next(int32_t iterator, uint64_t& primary) = 0;

         virtual int32_t db_idx_double_previous(int32_t iterator, uint64_t& primary) = 0;

         /**
          * interface for long double secondary
          */
         virtual int32_t db_idx_long_double_store(uint64_t scope, uint64_t table, account_name payer, uint64_t id,
                                                  const float128_t& secondary) = 0;

         virtual void db_idx_long_double_update(int32_t iterator, account_name payer, const float128_t& secondary) = 0;

         virtual void db_idx_long_double_remove(int32_t iterator) = 0;

         virtual int32_t db_idx_long_double_find_secondary(uint64_t code, uint64_t scope, uint64_t table,
                                                           const float128_t& secondary, uint64_t& primary) = 0;

         virtual int32_t db_idx_long_double_find_primary(uint64_t code, uint64_t scope, uint64_t table,
                                                         float128_t& secondary, uint64_t primary) = 0;

         virtual int32_t db_idx_long_double_lowerbound(uint64_t code, uint64_t scope, uint64_t table,
                                                       float128_t& secondary, uint64_t& primary) = 0;

         virtual int32_t db_idx_long_double_upperbound(uint64_t code, uint64_t scope, uint64_t table,
                                                       float128_t& secondary, uint64_t& primary) = 0;

         virtual int32_t db_idx_long_double_end(uint64_t code, uint64_t scope, uint64_t table) = 0;

         virtual int32_t db_idx_long_double_next(int32_t iterator, uint64_t& primary) = 0;

         virtual int32_t db_idx_long_double_previous(int32_t iterator, uint64_t& primary) = 0;

         static std::string table_event(name code, name scope, name table);
         static std::string table_event(name code, name scope, name table, name qualifier);
         static void log_insert_table(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table, account_name payer);
         static void log_remove_table(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table, account_name payer);
         static void log_row_insert(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                                    account_name payer, account_name primkey, const char* buffer, size_t buffer_size);
         static void log_row_update(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                                    account_name old_payer, account_name new_payer, account_name primkey,
                                    const char* old_buffer, size_t old_buffer_size, const char* new_buffer, size_t new_buffer_size);
         static void log_row_remove(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                                    account_name payer, account_name primkey, const char* buffer, size_t buffer_size);
         static storage_usage_trace add_table_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace rem_table_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace row_add_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace row_update_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace row_update_add_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace row_update_rem_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace row_rem_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace secondary_add_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace secondary_rem_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace secondary_update_add_trace(uint32_t action_id, std::string&& event_id);
         static storage_usage_trace secondary_update_rem_trace(uint32_t action_id, std::string&& event_id);
         void update_db_usage(const account_name& payer, int64_t delta, const storage_usage_trace& trace);
         apply_context& context;
         name           receiver;
      };

      std::unique_ptr<db_context> create_db_chainbase_context(apply_context& context, name receiver);
      std::unique_ptr<db_context> create_db_rocksdb_context(apply_context& context, name receiver,
                                                            eosio::session::session_variant<eosio::session::session<eosio::session::rocksdb_t>, eosio::session::session<eosio::session::session<eosio::session::rocksdb_t>>> session);

}}} // ns eosio::chain::backing_store
