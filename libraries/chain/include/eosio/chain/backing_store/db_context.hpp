#pragma once

#include <eosio/chain/name.hpp>
#include <memory>
#include <stdint.h>

namespace chainbase {
   class database;
}

namespace eosio {
   namespace session {
      template<typename Parent>
      class session;

      template <typename... T>
      class session_variant;
   }
namespace chain {

      class apply_context;

namespace backing_store { namespace db_context {
         std::string table_event(name code, name scope, name table);
         std::string table_event(name code, name scope, name table, name qualifier);
         void log_insert_table(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table, account_name payer);
         void log_remove_table(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table, account_name payer);
         void log_row_insert(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                             account_name payer, account_name primkey, const char* buffer, size_t buffer_size);
         void log_row_update(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                             account_name old_payer, account_name new_payer, account_name primkey,
                             const char* old_buffer, size_t old_buffer_size, const char* new_buffer, size_t new_buffer_size);
         void log_row_remove(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                             account_name payer, account_name primkey, const char* buffer, size_t buffer_size);
         storage_usage_trace add_table_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace rem_table_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace row_add_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace row_update_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace row_update_add_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace row_update_rem_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace row_rem_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace secondary_add_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace secondary_rem_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace secondary_update_add_trace(uint32_t action_id, std::string&& event_id);
         storage_usage_trace secondary_update_rem_trace(uint32_t action_id, std::string&& event_id);
}}}} // ns eosio::chain::backing_store::db_context
