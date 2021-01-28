#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/backing_store/db_context.hpp>

namespace eosio { namespace chain { namespace backing_store {

std::string db_context::table_event(name code, name scope, name table) {
   return STORAGE_EVENT_ID("${code}:${scope}:${table}",
                           ("code", code)
                           ("scope", scope)
                           ("table", table)
   );
}

std::string db_context::table_event(name code, name scope, name table, name qualifier) {
   return STORAGE_EVENT_ID("${code}:${scope}:${table}:${qualifier}",
                           ("code", code)
                           ("scope", scope)
                           ("table", table)
                           ("qualifier", qualifier)
   );
}

void db_context::log_insert_table(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table, account_name payer) {
   fc_dlog(deep_mind_logger, "TBL_OP INS ${action_id} ${code} ${scope} ${table} ${payer}",
      ("action_id", action_id)
      ("code", code)
      ("scope", scope)
      ("table", table)
      ("payer", payer)
   );
}

void db_context::log_remove_table(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table, account_name payer) {
   fc_dlog(deep_mind_logger, "TBL_OP REM ${action_id} ${code} ${scope} ${table} ${payer}",
      ("action_id", action_id)
      ("code", code)
      ("scope", scope)
      ("table", table)
      ("payer", payer)
   );
}

void db_context::log_row_insert(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                                 account_name payer, account_name primkey, const char* buffer, size_t buffer_size) {
   fc_dlog(deep_mind_logger, "DB_OP INS ${action_id} ${payer} ${table_code} ${scope} ${table_name} ${primkey} ${ndata}",
      ("action_id", action_id)
      ("payer", payer)
      ("table_code", code)
      ("scope", scope)
      ("table_name", table)
      ("primkey", primkey)
      ("ndata", fc::to_hex(buffer, buffer_size))
   );
}

void db_context::log_row_update(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                                 account_name old_payer, account_name new_payer, account_name primkey,
                                 const char* old_buffer, size_t old_buffer_size, const char* new_buffer, size_t new_buffer_size) {
   fc_dlog(deep_mind_logger, "DB_OP UPD ${action_id} ${opayer}:${npayer} ${table_code} ${scope} ${table_name} ${primkey} ${odata}:${ndata}",
      ("action_id", action_id)
      ("opayer", old_payer)
      ("npayer", new_payer)
      ("table_code", code)
      ("scope", scope)
      ("table_name", table)
      ("primkey", primkey)
      ("odata", to_hex(old_buffer, old_buffer_size))
      ("ndata", to_hex(new_buffer, new_buffer_size))
   );
}

void db_context::log_row_remove(fc::logger& deep_mind_logger, uint32_t action_id, name code, name scope, name table,
                                  account_name payer, account_name primkey, const char* buffer, size_t buffer_size) {
   fc_dlog(deep_mind_logger, "DB_OP REM ${action_id} ${payer} ${table_code} ${scope} ${table_name} ${primkey} ${odata}",
      ("action_id", action_id)
      ("payer", payer)
      ("table_code", code)
      ("scope", scope)
      ("table_name", table)
      ("primkey", primkey)
      ("odata", fc::to_hex(buffer, buffer_size))
   );
}

storage_usage_trace db_context::add_table_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "table", "add", "create_table");
}

storage_usage_trace db_context::rem_table_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "table", "remove", "remove_table");
}

storage_usage_trace db_context::row_add_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "table_row", "add", "primary_index_add");
}

storage_usage_trace db_context::row_update_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "table_row", "update", "primary_index_update");
}

storage_usage_trace db_context::row_update_add_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "table_row", "add", "primary_index_update_add_new_payer");
}

storage_usage_trace db_context::row_update_rem_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "table_row", "remove", "primary_index_update_remove_old_payer");
}

storage_usage_trace db_context::row_rem_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "table_row", "remove", "primary_index_remove");
}

storage_usage_trace db_context::secondary_add_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "secondary_index", "add", "secondary_index_add");
}

storage_usage_trace db_context::secondary_rem_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "secondary_index", "remove", "secondary_index_remove");
}

storage_usage_trace db_context::secondary_update_add_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "secondary_index", "add", "secondary_index_update_add_new_payer");
}

storage_usage_trace db_context::secondary_update_rem_trace(uint32_t action_id, std::string&& event_id) {
   return storage_usage_trace(action_id, std::move(event_id), "secondary_index", "remove", "secondary_index_update_remove_old_payer");
}

void db_context::update_db_usage( const account_name& payer, int64_t delta, const storage_usage_trace& trace ) {
   context.update_db_usage(payer, delta, trace);
}

}}} // namespace eosio::chain::backing_store
