#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/cursor_cache.hpp>

#include <cyberway/chain/cyberway_contract_types.hpp>
#include <cyberway/chain/cyberway_contract.hpp>

#include <assert.h>

#include <fc/io/json.hpp>
#include <cyberway/chaindb/names.hpp>

namespace eosio { namespace chain {

    namespace chaindb = cyberway::chaindb;

    using chaindb::cursor_t;
    using chaindb::account_name_t;
    using chaindb::scope_name_t;
    using chaindb::table_name_t;
    using chaindb::index_name_t;
    using chaindb::primary_key_t;
    using chaindb::cursor_kind;

    class chaindb_api : public context_aware_api {
    public:
        using context_aware_api::context_aware_api;

        cursor_t chaindb_clone(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            auto cloned_find_info = context.cursors_guard->get(code, cursor).clone();
            return context.cursors_guard->add(code, std::move(cloned_find_info));
        }

        void chaindb_close(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            context.cursors_guard->remove(code, cursor);
        }

        cursor_t chaindb_lower_bound(
            account_name_t code, scope_name_t scope, table_name_t table, index_name_t index,
            array_ptr<const char> key, size_t size
        ) {
            auto cursor = context.chaindb.lower_bound({code, scope, table, index}, cursor_kind::ManyRecords, key, size);
            return context.cursors_guard->add(code, std::move(cursor));
        }

        cursor_t chaindb_lower_bound_pk(
            account_name_t code, scope_name_t scope, table_name_t table, primary_key_t pk
        ) {
            auto cursor = context.chaindb.lower_bound({code, scope, table}, cursor_kind::ManyRecords, pk);
            return context.cursors_guard->add(code, std::move(cursor));
        }

        cursor_t chaindb_upper_bound(
            account_name_t code, scope_name_t scope, table_name_t table, index_name_t index,
            array_ptr<const char> key, size_t size
        ) {
            auto cursor = context.chaindb.upper_bound({code, scope, table, index}, key, size);
            return context.cursors_guard->add(code, std::move(cursor));
        }

        cursor_t chaindb_upper_bound_pk(
            account_name_t code, scope_name_t scope, table_name_t table, primary_key_t pk
        ) {
            auto cursor = context.chaindb.upper_bound({code, scope, table}, pk);
            return context.cursors_guard->add(code, std::move(cursor));
        }

        cursor_t chaindb_locate_to(
            account_name_t code, scope_name_t scope, table_name_t table, index_name_t index,
            primary_key_t pk, array_ptr<const char> key, size_t size
        ) {
            auto cursor = context.chaindb.locate_to({code, scope, table, index}, key, size, pk);
            return context.cursors_guard->add(code, std::move(cursor));
        }

        cursor_t chaindb_begin(
            account_name_t code, scope_name_t scope, table_name_t table, index_name_t index
        ) {
            auto cursor = context.chaindb.begin({code, scope, table, index});
            return context.cursors_guard->add(code, std::move(cursor));
        }

        cursor_t chaindb_end(
            account_name_t code, scope_name_t scope, table_name_t table, index_name_t index
        ) {
            auto cursor = context.chaindb.end({code, scope, table, index});
            return context.cursors_guard->add(code, std::move(cursor));
        }

        primary_key_t chaindb_current(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.current({code, cursor});
        }

        primary_key_t chaindb_next(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            auto pk = context.chaindb.next({code, cursor});
            return pk;
        }

        primary_key_t chaindb_prev(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it
            return context.chaindb.prev({code, cursor});
        }

        int32_t chaindb_datasize(account_name_t code, cursor_t cursor) {
            // cursor is already opened -> no reason to check ABI for it

            assert(context.chaindb_cache);
            auto& chaindb_cache = *context.chaindb_cache;

            chaindb::cursor_request request{code, cursor};

            chaindb_cache.cache_code   = code;
            chaindb_cache.cache_cursor = cursor;

            chaindb_cache.cache = context.chaindb.get_cache_object(request, true);

            if (chaindb_cache.cache) {
                auto& blob = chaindb_cache.cache->blob();
                CYBERWAY_ASSERT(blob.size() < 1024 * 1024, cyberway::chaindb::invalid_data_size_exception,
                    "Wrong data size ${data_size}", ("object_size", blob.size()));
                return static_cast<int32_t>(blob.size());
            } else {
                chaindb_cache.reset();
            }
            return 0;
        }

        primary_key_t chaindb_data(account_name_t code, cursor_t cursor, array_ptr<char> data, size_t size) {
            // cursor is already opened -> no reason to check ABI for it

            assert(context.chaindb_cache);
            auto& chaindb_cache = *context.chaindb_cache;

            CYBERWAY_ASSERT(chaindb_cache.cache_code == code && chaindb_cache.cache_cursor == cursor,
                cyberway::chaindb::invalid_data_exception,
                "Data is requested for the wrong cursor ${code}.${cursor}",
                ("code", account_name(code))("cursor", cursor));

            auto blob_size = chaindb_cache.cache->blob().size();

            CYBERWAY_ASSERT(blob_size == size, cyberway::chaindb::invalid_data_size_exception,
                "Wrong data size (${data_size} != ${object_size}) for the cursor ${code}.${cursor}",
                ("data_size", size)("object_size", blob_size)("code", account_name(code))("cursor", cursor));

            auto& blob = chaindb_cache.cache->blob();
            std::memcpy(data.value, blob.data(), blob.size());
            return chaindb_cache.cache->pk();
        }

        int32_t chaindb_service(account_name_t code, cursor_t cursor, array_ptr<char> data, size_t size) {
            // cursor is already opened -> no reason to check ABI for it

            assert(context.chaindb_cache);
            auto& chaindb_cache = *context.chaindb_cache;

            CYBERWAY_ASSERT(chaindb_cache.cache_code == code && chaindb_cache.cache_cursor == cursor,
                cyberway::chaindb::invalid_data_exception,
                "Service is requested for the wrong cursor ${code}.${cursor}",
                 ("code", account_name(code))("cursor", cursor));

            auto service = context.chaindb_cache->cache->service();
            auto s = fc::raw::pack_size(service);
            if (size == 0) {
                return s;
            }

            chaindb_cache.reset();

            // allow to extend structure in the future

            vector<char> pack_buffer;
            pack_buffer.resize(size);
            datastream<char*> ds(pack_buffer.data(), s);
            fc::raw::pack(ds, service);

            s = std::min(s, size);
            std::memset(data, 0, size);
            std::memcpy(data, pack_buffer.data(), s);

            return s;
        }

        primary_key_t chaindb_available_primary_key(account_name_t code, scope_name_t scope, table_name_t table) {
            return context.chaindb.available_pk({code, scope, table});
        }

        void validate_db_access_violation(const account_name code) {
            EOS_ASSERT(code == context.receiver || (code.empty() && context.privileged),
                table_access_violation, "db access violation");
        }

        int32_t chaindb_insert(
            account_name_t code, scope_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            EOS_ASSERT(account_name(payer) != account_name(), invalid_table_payer,
                "must specify a valid account to pay for new record");
            validate_db_access_violation(code);
            auto delta = context.chaindb.insert({code, scope, table}, context.get_storage_payer(payer), pk, data, size);
            return static_cast<int32_t>(delta);
        }

        int32_t chaindb_update(
            account_name_t code, scope_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            validate_db_access_violation(code);
            auto delta = context.chaindb.update({code, scope, table}, context.get_storage_payer(payer), pk, data, size);
            return static_cast<int32_t>(delta);
        }

        int32_t chaindb_delete(
            account_name_t code, scope_name_t scope, table_name_t table, account_name_t payer, primary_key_t pk
        ) {
            validate_db_access_violation(code);
            auto delta = context.chaindb.remove({code, scope, table}, context.get_storage_payer(payer), pk);
            return static_cast<int32_t>(delta);
        }

        void chaindb_ram_state(
            account_name_t code, scope_name_t scope, table_name_t table, primary_key_t pk, bool in_ram
        ) {
            validate_db_access_violation(code);

            chaindb::table_request request{code, scope, table};

            auto cache_ptr = context.chaindb.get_cache_object(request, pk, false);

            EOS_ASSERT(cache_ptr, eosio::chain::object_query_exception,
                "Object with the primary key ${pk} doesn't exist in the table ${table}:${scope}",
                ("pk", pk)("table", chaindb::get_full_table_name(request))("scope", scope));

            auto& service = cache_ptr->service();

            EOS_ASSERT(in_ram != service.in_ram, eosio::chain::object_ram_state_exception,
                "Object with the primary key ${pk} in the table ${table}:${scope} already has RAM state = ${state}",
                ("pk", pk)("table", chaindb::get_full_table_name(request))("scope", scope));

            auto info = context.get_storage_payer(service.payer, service.payer);
            info.in_ram  = in_ram;
            context.chaindb.change_ram_state(*cache_ptr.get(), info);
        }

    }; // class chaindb_api

    REGISTER_INTRINSICS( chaindb_api,
        (chaindb_clone,       int(int64_t, int)  )
        (chaindb_close,       void(int64_t, int) )

        (chaindb_begin,       int(int64_t, int64_t, int64_t, int64_t) )
        (chaindb_end,         int(int64_t, int64_t, int64_t, int64_t) )

        (chaindb_locate_to,   int(int64_t, int64_t, int64_t, int64_t, int64_t, int, int) )

        (chaindb_lower_bound,    int(int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_lower_bound_pk, int(int64_t, int64_t, int64_t, int64_t)           )
        (chaindb_upper_bound,    int(int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_upper_bound_pk, int(int64_t, int64_t, int64_t, int64_t)           )

        (chaindb_current,     int64_t(int64_t, int) )
        (chaindb_next,        int64_t(int64_t, int) )
        (chaindb_prev,        int64_t(int64_t, int) )

        (chaindb_datasize,    int(int64_t, int)               )
        (chaindb_data,        int64_t(int64_t, int, int, int) )
        (chaindb_service,     int(int64_t, int, int, int)     )

        (chaindb_available_primary_key, int64_t(int64_t, int64_t, int64_t) )

        (chaindb_insert,      int(int64_t, int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_update,      int(int64_t, int64_t, int64_t, int64_t, int64_t, int, int) )
        (chaindb_delete,      int(int64_t, int64_t, int64_t, int64_t, int64_t)           )

        (chaindb_ram_state,   void(int64_t, int64_t, int64_t, int64_t, int) )
    );

} } /// eosio::chain
