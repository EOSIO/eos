#pragma once

#include <string>
#include <memory>

#include <cyberway/chaindb/driver_interface.hpp>

namespace cyberway { namespace chaindb {

    class journal;

    struct mongodb_driver_impl;

    class mongodb_driver final: public driver_interface {
    public:
        mongodb_driver(journal&, string, string);
        ~mongodb_driver();

        std::vector<table_def> db_tables(const account_name& code) const override;
        void create_index(const index_info&) const override;
        void drop_index(const index_info&) const override;
        void drop_table(const table_info&) const override;

        void drop_db() const override;

        const cursor_info& clone(const cursor_request&) const override;

        void close(const cursor_request&) const override;
        void close_code_cursors(const account_name& code) const override;

        void apply_code_changes(const account_name& code) const override;
        void apply_all_changes() const override;

        void skip_pk(const table_info&, primary_key_t) const override;

        cursor_info& lower_bound(index_info, variant key) const override;
        cursor_info& upper_bound(index_info, variant key) const override;
        cursor_info& locate_to(index_info, variant key, primary_key_t) const override;

        cursor_info& begin(index_info) const override;
        cursor_info& end(index_info) const override;

        cursor_info& cursor(const cursor_request&) const override;
        cursor_info& current(const cursor_info&) const override;
        cursor_info& next(const cursor_info&) const override;
        cursor_info& prev(const cursor_info&) const override;

              object_value  object_by_pk(const table_info&, primary_key_t) const override;
        const object_value& object_at_cursor(const cursor_info&, bool) const override;

        primary_key_t available_pk(const table_info&) const override;

    private:
        std::unique_ptr<mongodb_driver_impl> impl_;
    }; // class mongodb_driver

} } // namespace cyberway::chaindb
