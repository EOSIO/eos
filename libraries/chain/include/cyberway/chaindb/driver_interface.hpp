#pragma once

#include <fc/variant.hpp>

#include <cyberway/chaindb/controller.hpp>

namespace cyberway { namespace chaindb {

    using fc::variant;

    using eosio::chain::bytes;
    using eosio::chain::field_name;

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
        cursor_t      id = invalid_cursor;
        index_info    index;
        primary_key_t pk = end_primary_key;
        bytes         blob; // serialized by controller value
    }; // struct cursor_info

    class driver_interface {
    public:
        virtual ~driver_interface();

        virtual const cursor_info& clone(const cursor_request&) = 0;

        virtual void close(const cursor_request&) = 0;
        virtual void close_all_cursors(const account_name& code) = 0;

        virtual void verify_table_structure(const table_info&, const microseconds&) = 0;

        virtual const cursor_info& lower_bound(index_info, variant key) = 0;
        virtual const cursor_info& upper_bound(index_info, variant key) = 0;
        virtual const cursor_info& find(index_info, primary_key_t, variant key) = 0;
        virtual const cursor_info& end(index_info) = 0;

        virtual const cursor_info& current(const cursor_info&) = 0;
        virtual const cursor_info& current(const cursor_request&) = 0;
        virtual const cursor_info& next(const cursor_request&) = 0;
        virtual const cursor_info& prev(const cursor_request&) = 0;

        virtual       variant  value(const table_info&, const primary_key_t) = 0;
        virtual const variant& value(const cursor_info&) = 0;

        virtual void set_blob(const cursor_info&, bytes blob) = 0;

        virtual primary_key_t available_primary_key(const table_info&) = 0;

        virtual primary_key_t insert(const table_info&, variant) = 0;
        virtual primary_key_t update(const table_info&, variant) = 0;
        virtual primary_key_t remove(const table_info&, primary_key_t) = 0;
    }; // class driver_interface

} } // namespace cyberway::chaindb