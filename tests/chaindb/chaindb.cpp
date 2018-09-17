#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <mongocxx/exception/logic_error.hpp>

#include "chaindb.h"

struct mongodb_ctx {
    mongocxx::instance mongo_inst;
    mongocxx::client mongo_conn;
};

static mongodb_ctx ctx;

int32_t chaindb_init(const char* uri_str) {
    try {
        mongocxx::uri uri{uri_str};
        ctx.mongo_conn = mongocxx::client{uri};
    } catch (mongocxx::exception & ex) {
        return 0;
    }
    return 1;
}

cursor_t chaindb_lower_bound(account_name_t code, account_name_t scope, table_name_t, index_name_t, void* key, size_t) {
    return invalid_cursor;
}

cursor_t chaindb_upper_bound(account_name_t code, account_name_t scope, table_name_t, index_name_t, void* key, size_t) {
    return invalid_cursor;
}

cursor_t chaindb_find(account_name_t code, account_name_t scope, table_name_t, index_name_t, primary_key_t, void* key, size_t) {
    return invalid_cursor;
}

cursor_t chaindb_end(account_name_t code, account_name_t scope, table_name_t, index_name_t) {
    return invalid_cursor;
}

void chaindb_close(cursor_t) {

}

primary_key_t chaindb_current(cursor_t) {
    return unset_primary_key;
}

primary_key_t chaindb_next(cursor_t) {
    return unset_primary_key;
}

primary_key_t chaindb_prev(cursor_t) {
    return unset_primary_key;
}

int32_t chaindb_datasize(cursor_t) {
    return 0;
}

primary_key_t chaindb_data(cursor_t, void* data, const size_t size) {
    return unset_primary_key;
}

primary_key_t chaindb_available_primary_key(account_name_t code, account_name_t scope, table_name_t table) {
    return 1;
}

cursor_t chaindb_insert(account_name_t code, account_name_t scope, table_name_t, primary_key_t, void* data, size_t) {
    return invalid_cursor;
}

primary_key_t chaindb_update(account_name_t code, account_name_t scope, table_name_t, primary_key_t, void* data, size_t) {
    return unset_primary_key;
}

primary_key_t chaindb_delete(account_name_t code, account_name_t scope, table_name_t, primary_key_t) {
    return unset_primary_key;
}