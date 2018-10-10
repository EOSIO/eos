#pragma once

#include <fc/variant.hpp>

#include <cyberway/chaindb/controller.hpp>

namespace cyberway { namespace chaindb {

    using fc::variant;

    using eosio::chain::bytes;
    using eosio::chain::table_def;
    using eosio::chain::field_name;
    using eosio::chain::index_def;

    class abi_info;

    struct table_info {
        const account_name code;
        const account_name scope;
        const abi_info*    abi      = nullptr;
        const table_def*   table    = nullptr;
        const field_name*  pk_field = nullptr;

        table_info(const account_name& code, const account_name& scope)
        : code(code), scope(scope)
        { }
    }; // struct table_info

    struct index_info: public table_info {
        const index_def* index = nullptr;

        using table_info::table_info;
    }; // struct index_info

    struct cursor_info {
        const cursor_t      id   = invalid_cursor;
        const primary_key_t pk   = unset_primary_key;
        const index_info    info;
    }; // struct cursor_info

    class driver_interface {
    public:
        virtual ~driver_interface();

        virtual const cursor_info& clone(cursor_t) = 0;

        virtual void close(cursor_t) = 0;
        virtual void close_all_cursors(const account_name& code) = 0;

        virtual void verify_table_structure(const table_info&, const microseconds&) = 0;

        virtual const cursor_info& lower_bound(index_info, const variant& key) = 0;
        virtual const cursor_info& upper_bound(index_info, const variant& key) = 0;
        virtual const cursor_info& find(index_info, primary_key_t, const variant& key) = 0;
        virtual const cursor_info& end(index_info) = 0;

        virtual const cursor_info& current(cursor_t) = 0;
        virtual const cursor_info& next(cursor_t) = 0;
        virtual const cursor_info& prev(cursor_t) = 0;

        virtual const variant& value(const primary_key_t) = 0;
        virtual const variant& value(const cursor_info&) = 0;
        virtual const bytes&   data(const cursor_info&) const = 0;
        virtual       bytes&   mutable_data(const cursor_info&) = 0;

        virtual primary_key_t available_primary_key(const table_info&) = 0;

        virtual primary_key_t insert(const table_info&, const variant&) = 0;
        virtual primary_key_t update(const table_info&, const variant&) = 0;
        virtual primary_key_t remove(const table_info&, primary_key_t) = 0;
    }; // class driver_interface

} } // namespace cyberway::chaindb