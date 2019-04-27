#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/cursor_cache.hpp>

#include <cyberway/chain/cyberway_contract_types.hpp>
#include <cyberway/chain/cyberway_contract.hpp>

#include <assert.h>

#include <fc/io/json.hpp>
#include <cyberway/chaindb/names.hpp>

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
            account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.lower_bound({code, scope, table}, pk).cursor;
        }

        cursor_t chaindb_upper_bound(
            account_name_t code, account_name_t scope, table_name_t table, index_name_t index,
            array_ptr<const char> key, size_t size
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.upper_bound({code, scope, table, index}, key, size).cursor;
        }

        cursor_t chaindb_upper_bound_pk(
            account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk
        ) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.upper_bound({code, scope, table}, pk).cursor;
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

            chaindb_cache.cache_code   = code;
            chaindb_cache.cache_cursor = cursor;

            chaindb_cache.cache = context.chaindb.get_cache_object({code, cursor}, true);

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
            std::memset(data, size, 0);
            std::memcpy(data, pack_buffer.data(), s);

            return s;
        }

        primary_key_t chaindb_available_primary_key(account_name_t code, account_name_t scope, table_name_t table) {
            context.lazy_init_chaindb_abi(code);
            return context.chaindb.available_pk({code, scope, table});
        }

        void validate_db_access_violation(const account_name code) {
            EOS_ASSERT(code == context.receiver || (code.empty() && context.privileged),
                table_access_violation, "db access violation");
        }

        int32_t chaindb_insert(
            account_name_t code, account_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            EOS_ASSERT(account_name(payer) != account_name(), invalid_table_payer,
                "must specify a valid account to pay for new record");
            validate_db_access_violation(code);
            context.lazy_init_chaindb_abi(code);
            auto delta = context.chaindb.insert({code, scope, table}, context.get_storage_payer(payer), pk, data, size);
            return static_cast<int32_t>(delta);
        }

        int32_t chaindb_update(
            account_name_t code, account_name_t scope, table_name_t table,
            account_name_t payer, primary_key_t pk, array_ptr<const char> data, size_t size
        ) {
            validate_db_access_violation(code);
            context.lazy_init_chaindb_abi(code);
            auto delta = context.chaindb.update({code, scope, table}, context.get_storage_payer(payer), pk, data, size);
            return static_cast<int32_t>(delta);
        }

        int32_t chaindb_delete(
            account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk
        ) {
            validate_db_access_violation(code);
            context.lazy_init_chaindb_abi(code);
            auto delta = context.chaindb.remove({code, scope, table}, context.get_storage_payer(), pk);
            return static_cast<int32_t>(delta);
        }

        void chaindb_ram_state(
            account_name_t code, account_name_t scope, table_name_t table, primary_key_t pk, bool in_ram
        ) {
            // This realization doesn't allow to call handler in system contract,
            //   but as result it works faster - is it good or not?
            // As an alternative, the account contract can send inline action by itself ...

            validate_db_access_violation(code);
            context.lazy_init_chaindb_abi(code);

            cyberway::chain::set_ram_state op;
            op.code   = code;
            op.scope  = scope;
            op.table  = table;
            op.pk     = pk;
            op.in_ram = in_ram;

            cyberway::chain::apply_set_ram_state(context, op, [&](const cyberway::chaindb::service_state& service) {
                if (context.privileged || (service.owner == context.receiver && service.payer == context.receiver)) {
                    return;
                } else if (service.owner == service.payer || !context.weak_require_authorization(service.payer)) {
                    context.require_authorization(service.owner);
                }
            });
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
        (chaindb_delete,      int(int64_t, int64_t, int64_t, int64_t)                    )

        (chaindb_ram_state,   void(int64_t, int64_t, int64_t, int64_t, int) )
    );

} } /// eosio::chain
