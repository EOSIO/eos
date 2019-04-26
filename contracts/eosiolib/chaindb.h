#pragma once

#include "types.hpp"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t table_name_t;
typedef uint64_t index_name_t;
typedef uint64_t account_name_t;
typedef uint64_t scope_t;

typedef int32_t cursor_t;

typedef uint64_t primary_key_t;
static constexpr primary_key_t end_primary_key = static_cast<primary_key_t>(-1);
static constexpr primary_key_t unset_primary_key = static_cast<primary_key_t>(-2);

cursor_t chaindb_begin(account_name_t code, scope_t scope, table_name_t, index_name_t);
cursor_t chaindb_end(account_name_t code, scope_t scope, table_name_t, index_name_t);
cursor_t chaindb_lower_bound(account_name_t code, scope_t scope, table_name_t, index_name_t, void* key, size_t);
cursor_t chaindb_lower_bound_pk(account_name_t code, scope_t scope, table_name_t, primary_key_t);
cursor_t chaindb_upper_bound(account_name_t code, scope_t scope, table_name_t, index_name_t, void* key, size_t);
cursor_t chaindb_upper_bound_pk(account_name_t code, scope_t scope, table_name_t, primary_key_t);
cursor_t chaindb_locate_to(account_name_t code, scope_t scope, table_name_t, index_name_t, primary_key_t, void* key, size_t);
cursor_t chaindb_clone(account_name_t code, cursor_t);

void chaindb_close(account_name_t code, cursor_t);

primary_key_t chaindb_current(account_name_t code, cursor_t);
primary_key_t chaindb_next(account_name_t code, cursor_t);
primary_key_t chaindb_prev(account_name_t code, cursor_t);

int32_t chaindb_datasize(account_name_t code, cursor_t);
primary_key_t chaindb_data(account_name_t code, cursor_t, void* data, const size_t size);
int32_t chaindb_service(account_name_t code, cursor_t, void* data, const size_t size);

primary_key_t chaindb_available_primary_key(account_name_t code, scope_t scope, table_name_t table);

int32_t chaindb_insert(account_name_t code, scope_t scope, account_name_t payer, table_name_t, primary_key_t, void* data, size_t);
int32_t chaindb_update(account_name_t code, scope_t scope, account_name_t payer, table_name_t, primary_key_t, void* data, size_t);
int32_t chaindb_delete(account_name_t code, scope_t scope, table_name_t, primary_key_t);

int32_t chaindb_ram_state(account_name_t code, scope_t scope, table_name_t, primary_key_t, bool);

#ifdef __cplusplus
}
#endif
