#pragma once

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/table_info.hpp>

#include <fc/variant.hpp>

namespace cyberway { namespace chaindb {

    struct cursor_info {
        cursor_t      id = invalid_cursor;
        index_info    index;
        primary_key_t pk = primary_key::End;
        object_value  object;
    }; // struct cursor_info

    class driver_interface {
    public:
        virtual ~driver_interface();

        virtual void drop_db() const = 0;

        virtual std::vector<table_def> db_tables(const account_name& code) const = 0;
        virtual void create_index(const index_info&) const = 0;
        virtual void drop_index(const index_info&) const = 0;
        virtual void drop_table(const table_info&) const = 0;

        virtual const cursor_info& clone(const cursor_request&) const = 0;

        virtual void close(const cursor_request&) const = 0;
        virtual void close_code_cursors(const account_name& code) const = 0;

        virtual void apply_all_changes() const = 0;
        virtual void apply_code_changes(const account_name& code) const = 0;

        virtual void skip_pk(const table_info&, primary_key_t) const = 0;

        virtual cursor_info& lower_bound(index_info, variant key) const = 0;
        virtual cursor_info& upper_bound(index_info, variant key) const = 0;
        virtual cursor_info& locate_to(index_info, variant key, primary_key_t) const = 0;

        virtual cursor_info& begin(index_info) const = 0;
        virtual cursor_info& end(index_info) const = 0;

        virtual cursor_info& cursor(const cursor_request&) const = 0;
        virtual cursor_info& current(const cursor_info&) const = 0;
        virtual cursor_info& next(const cursor_info&) const = 0;
        virtual cursor_info& prev(const cursor_info&) const = 0;

        virtual       object_value  object_by_pk(const table_info&, primary_key_t) const = 0;
        virtual const object_value& object_at_cursor(const cursor_info&, bool with_decors) const = 0;

        virtual primary_key_t available_pk(const table_info&) const = 0;
    }; // class driver_interface

} } // namespace cyberway::chaindb
