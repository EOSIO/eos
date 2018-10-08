#pragma once

#include <string>
#include <memory>

#include <fc/variant.hpp>

#include <eosio/chain/abi_def.hpp>

#include <cyberway/chaindb/chaindb_common.hpp>

namespace eosio { namespace chain {
    class apply_context;
} } // namespace eosio::chain

namespace cyberway { namespace chaindb {
    using std::string;

    using fc::variant;

    using eosio::chain::account_name;
    using eosio::chain::apply_context;
    using eosio::chain::abi_def;
    using eosio::chain::table_def;
    using eosio::chain::index_def;

    using table_name_t = eosio::chain::table_name::value_type;
    using index_name_t = table_name_t;
    using account_name_t = eosio::chain::account_name::value_type;

    using cursor_t = int32_t;
    static constexpr cursor_t invalid_cursor = (-1);

    using primary_key_t = uint64_t;
    static constexpr primary_key_t unset_primary_key = (-1);

    struct index_request {
        const account_name_t code;
        const account_name_t scope;
        const table_name_t table;
        const index_name_t index;
    }; // struct index_request

    struct table_request {
        const account_name_t code;
        const account_name_t scope;
        const table_name_t table;
    }; // struct table_request

    struct table_info {
        const account_name_t code = 0;
        const account_name_t scope = 0;
        const table_def* table = nullptr;

        table_info() = default;
        table_info(const table_request& src, const table_def* table)
        : code(src.code), scope(src.scope), table(table)
        { }
    }; // struct table_info

    struct index_info {
        const account_name_t code = 0;
        const account_name_t scope = 0;
        const table_def* table = nullptr;
        const index_def* index = nullptr;

        index_info() = default;
        index_info(const table_info& src, const index_def* idx)
        : code(src.code), scope(src.scope), table(src.table), index(idx)
        { }
    }; // struct index_info

    class chaindb_interface {
    public:
        virtual ~chaindb_interface();

        virtual void           close(cursor_t) = 0;
        virtual void           close_all_cursors(account_name_t code) = 0;

        virtual cursor_t       lower_bound(const index_info&, const variant& key) = 0;
        virtual cursor_t       upper_bound(const index_info&, const variant& key) = 0;
        virtual cursor_t       find(const index_info&, primary_key_t, const variant& key) = 0;
        virtual cursor_t       end(const index_info&) = 0;
        virtual cursor_t       clone(cursor_t) = 0;

        virtual primary_key_t  current(cursor_t) = 0;
        virtual primary_key_t  next(cursor_t) = 0;
        virtual primary_key_t  prev(cursor_t) = 0;

        virtual const variant& data(cursor_t) = 0;

        virtual primary_key_t  available_primary_key(const table_info&) = 0;

        virtual cursor_t       insert(const table_info&, const variant&) = 0;
        virtual primary_key_t  update(const table_info&, const variant&) = 0;
        virtual primary_key_t  remove(const table_info&, primary_key_t) = 0;
    }; // class chaindb_interface

    class chaindb_controller {
    public:
        chaindb_controller(chaindb_type, string);
        ~chaindb_controller();

        void add_code(const account_name&, abi_def);
        void remove_code(const account_name&);
        bool has_code(const account_name&);

        void close(cursor_t);
        void close_all_cursors(const account_name&);

        cursor_t lower_bound(const index_request&, const char* key, size_t);
        cursor_t upper_bound(const index_request&, const char* key, size_t);
        cursor_t find(const index_request&, primary_key_t, const char* key, size_t);
        cursor_t end(const index_request&);
        cursor_t clone(cursor_t);

        primary_key_t current(cursor_t);
        primary_key_t next(cursor_t);
        primary_key_t prev(cursor_t);

        int32_t       datasize(cursor_t);
        primary_key_t data(cursor_t, const char* data, size_t size);

        primary_key_t available_primary_key(const table_request&);

        cursor_t      insert(apply_context&, const table_request&, account_name_t, primary_key_t, const char* data, size_t);
        primary_key_t update(apply_context&, const table_request&, account_name_t, primary_key_t, const char* data, size_t);
        primary_key_t remove(apply_context&, const table_request&, primary_key_t);

    private:
        struct controller_impl_;
        std::unique_ptr<controller_impl_> impl_;
    }; // class chaindb_controller

} } // namespace cyberway::chaindb