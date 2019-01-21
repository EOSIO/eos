#include <cyberway/chaindb/controller.hpp>

namespace eosio { namespace chain {

    using cyberway::chaindb::cursor_t;
    using cyberway::chaindb::account_name_t;
    using cyberway::chaindb::table_name_t;
    using cyberway::chaindb::index_name_t;
    using cyberway::chaindb::primary_key_t;

    class chaindb_api : public context_aware_api {
    public:
        using context_aware_api::context_aware_api;

        cursor_t chaindb_clone(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.clone({code, cursor}).cursor;
        }

        void chaindb_close(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            context.chaindb.close({code, cursor});
        }

        cursor_t chaindb_lower_bound(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index,
            array_ptr<const char> key, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.lower_bound({code, scope, table, index}, key, size).cursor;
        }

        cursor_t chaindb_upper_bound(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index,
            array_ptr<const char> key, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.upper_bound({code, scope, table, index}, key, size).cursor;
        }

        cursor_t chaindb_opt_find_by_pk(account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.opt_find_by_pk({code, scope, table}, pk).cursor;
        }

        cursor_t chaindb_find(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index,
            primary_key_t pk, array_ptr<const char> key, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.find({code, scope, table, index}, pk, key, size).cursor;
        }

        cursor_t chaindb_end(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.end({code, scope, table, index}).cursor;
        }

        primary_key_t chaindb_current(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.current({code, cursor});
        }

        primary_key_t chaindb_next(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.next({code, cursor});
        }

        primary_key_t chaindb_prev(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.prev({code, cursor});
        }

        int32_t chaindb_datasize(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.datasize({code, cursor});
        }

        primary_key_t chaindb_data(account_name_t code, cursor_t cursor, array_ptr<const char> data, size_t size) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.data({code, cursor}, data, size);
        }

        primary_key_t chaindb_available_primary_key(account_name_t code, account_name_t scope, table_name_t table) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.available_pk({code, scope, table});
        }

        primary_key_t chaindb_insert(
            account_name_t code, account_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.insert(context, {code, scope, table}, payer, pk, data, size);
        }

        primary_key_t chaindb_update(
            account_name_t code, account_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.update(context, {code, scope, table}, payer, pk, data, size);
        }

        primary_key_t chaindb_delete(
            account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.remove(context, {code, scope, table}, pk);
        }

    }; // class chaindb_api

    REGISTER_INTRINSICS( chaindb_api,
        (chaindb_clone,       int(int64_t, int)  )
        (chaindb_close,       void(int64_t, int) )

        (chaindb_opt_find_by_pk, int(int64_t, int64_t, int64_t, int64_t))

        (chaindb_lower_bound, int(int64_t, int64_t, int64_t, int64_t, int, int)          )
        (chaindb_upper_bound, int(int64_t, int64_t, int64_t, int64_t, int, int)          )
        (chaindb_find,        int(int64_t, int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_end,         int(int64_t, int64_t, int64_t, int64_t)                    )

        (chaindb_current,     int64_t(int64_t, int) )
        (chaindb_next,        int64_t(int64_t, int) )
        (chaindb_prev,        int64_t(int64_t, int) )

        (chaindb_datasize,    int(int64_t, int)               )
        (chaindb_data,        int64_t(int64_t, int, int, int) )

        (chaindb_available_primary_key, int64_t(int64_t, int64_t, int64_t) )

        (chaindb_insert,      int64_t(int64_t, int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_update,      int64_t(int64_t, int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_delete,      int64_t(int64_t, int64_t, int64_t, int64_t)                    )
    );

} } /// eosio::chain
