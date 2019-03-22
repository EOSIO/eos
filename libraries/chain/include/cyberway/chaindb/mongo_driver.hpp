#pragma once

#include <string>
#include <memory>

#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/journal.hpp>

namespace cyberway { namespace chaindb {

    class mongodb_driver final: public driver_interface {
    public:
        mongodb_driver(journal&, string, string);
        ~mongodb_driver();

        std::vector<table_def> db_tables(const account_name& code) const override;
        void create_index(const index_info&) const override;
        void drop_index(const index_info&) const override;
        void drop_table(const table_info&) const override;

        void drop_db() override;

        const cursor_info& clone(const cursor_request&) override;

        void close(const cursor_request&) override;
        void close_code_cursors(const account_name& code) override;

        void apply_code_changes(const account_name& code) override;
        void apply_all_changes() override;

        const cursor_info& lower_bound(index_info, variant key) override;
        const cursor_info& upper_bound(index_info, variant key) override;
        const cursor_info& find(index_info, primary_key_t, variant key) override;
        const cursor_info& opt_find_by_pk(index_info, primary_key_t, variant key) override;

        const cursor_info& begin(index_info) override;
        const cursor_info& end(index_info) override;

        const cursor_info& current(const cursor_info&) override;
        const cursor_info& current(const cursor_request&) override;
        const cursor_info& next(const cursor_request&) override;
        const cursor_info& next(const cursor_info&) override;
        const cursor_info& prev(const cursor_request&) override;
        const cursor_info& prev(const cursor_info&) override;

              object_value  object_by_pk(const table_info&, primary_key_t) override;
        const object_value& object_at_cursor(const cursor_info&) override;
              void          set_blob(const cursor_info&, bytes blob) override;

        primary_key_t available_pk(const table_info&) override;

    private:
        struct mongodb_impl_;
        std::unique_ptr<mongodb_impl_> impl_;
    }; // class mongodb_driver

} } // namespace cyberway::chaindb
