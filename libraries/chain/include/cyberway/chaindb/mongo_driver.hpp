#pragma once

#include <string>
#include <memory>

#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/journal.hpp>

namespace cyberway { namespace chaindb {

    class mongodb_driver final: public driver_interface {
    public:
        mongodb_driver(journal&, const std::string&);
        ~mongodb_driver();

        void drop_db() override;

        const cursor_info& clone(const cursor_request&) override;

        void close(const cursor_request&) override;
        void close_code_cursors(const account_name& code) override;

        void apply_code_changes(const account_name& code) override;
        void apply_all_changes() override;

        void verify_table_structure(const table_info&, const microseconds&) override;

        const cursor_info& lower_bound(index_info, variant key) override;
        const cursor_info& upper_bound(index_info, variant key) override;
        const cursor_info& find(index_info, primary_key_t, variant key) override;
        const cursor_info& opt_find_by_pk(index_info, primary_key_t, variant key) override;

        const cursor_info& end(index_info) override;

        const cursor_info& current(const cursor_info&) override;
        const cursor_info& current(const cursor_request&) override;
        const cursor_info& next(const cursor_request&) override;
        const cursor_info& prev(const cursor_request&) override;

              variant  value(const table_info&, primary_key_t) override;
        const variant& value(const cursor_info&) override;
              void     set_blob(const cursor_info&, bytes blob) override;

        primary_key_t available_pk(const table_info&) override;

    private:
        struct mongodb_impl_;
        std::unique_ptr<mongodb_impl_> impl_;
    }; // class mongodb_driver

} } // namespace cyberway::chaindb
