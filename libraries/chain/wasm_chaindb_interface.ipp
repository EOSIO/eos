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

        cursor_t chaindb_lower_bound_pk(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index, primary_key_t pk
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.lower_bound({code, scope, table, index}, pk).cursor;
        }

        cursor_t chaindb_upper_bound(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index,
            array_ptr<const char> key, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.upper_bound({code, scope, table, index}, key, size).cursor;
        }

        cursor_t chaindb_upper_bound_pk(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index, primary_key_t pk
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.upper_bound({code, scope, table, index}, pk).cursor;
        }

        cursor_t chaindb_locate_to(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index,
            primary_key_t pk, array_ptr<const char> key, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.locate_to({code, scope, table, index}, key, size, pk).cursor;
        }

        cursor_t chaindb_begin(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.begin({code, scope, table, index}).cursor;
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

            auto  cache = context.chaindb.get_cache_object({code, cursor}, true);
            auto& blob  = cache->blob();
            CYBERWAY_ASSERT(blob.size() < 1024 * 1024, cyberway::chaindb::invalid_data_size_exception,
                "Wrong data size ${data_size}", ("object_size", blob.size()));
            return static_cast<int32_t>(blob.size());
        }

        primary_key_t chaindb_data(account_name_t code, cursor_t cursor, array_ptr<char> data, size_t size) {
            // cursor is already opened -> no reason to check ABI for it

            auto  cache = context.chaindb.get_cache_object({code, cursor}, false);
            auto& blob  = cache->blob();
            CYBERWAY_ASSERT(blob.size() == size, cyberway::chaindb::invalid_data_size_exception,
                "Wrong data size (${data_size} != ${object_size})",
                ("data_size", size)("object_size", blob.size()));

            std::memcpy(data.value, blob.data(), blob.size());
            return cache->pk();
        }

        primary_key_t chaindb_available_primary_key(account_name_t code, account_name_t scope, table_name_t table) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.available_pk({code, scope, table});
        }

        void validate_db_access_violation(const account_name code) {
            EOS_ASSERT(code == context.receiver || (code.empty() && context.privileged),
                table_access_violation, "db access violation");
        }

        primary_key_t chaindb_insert(
            account_name_t code, account_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            EOS_ASSERT(account_name(payer) != account_name(), invalid_table_payer,
                "must specify a valid account to pay for new record");
            validate_db_access_violation(code);
            context.lazy_init_chaindb_abi(code);
            context.chaindb.insert({code, scope, table}, {context, payer}, pk, data, size);
            return pk;
        }

        primary_key_t chaindb_update(
            account_name_t code, account_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            validate_db_access_violation(code);
            context.lazy_init_chaindb_abi(code);
            context.chaindb.update({code, scope, table}, {context, payer}, pk, data, size);
            return pk;
        }

        primary_key_t chaindb_delete(
            account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk
        ) {
            validate_db_access_violation(code);
            context.lazy_init_chaindb_abi(code);
            context.chaindb.remove({code, scope, table}, {context}, pk);
            return pk;
        }

    }; // class chaindb_api

    REGISTER_INTRINSICS( chaindb_api,
        (chaindb_clone,       int(int64_t, int)  )
        (chaindb_close,       void(int64_t, int) )

        (chaindb_begin,       int(int64_t, int64_t, int64_t, int64_t) )
        (chaindb_end,         int(int64_t, int64_t, int64_t, int64_t) )

        (chaindb_locate_to,   int(int64_t, int64_t, int64_t, int64_t, int64_t, int, int) )

        (chaindb_lower_bound,    int(int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_lower_bound_pk, int(int64_t, int64_t, int64_t, int64_t, int64_t)  )
        (chaindb_upper_bound,    int(int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_upper_bound_pk, int(int64_t, int64_t, int64_t, int64_t, int64_t)  )

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
