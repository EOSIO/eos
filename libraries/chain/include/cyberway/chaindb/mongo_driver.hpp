#pragma once

#include <string>
#include <memory>

#include <cyberway/chaindb/driver_interface.hpp>

namespace cyberway { namespace chaindb {

    class mongodb_driver final: public driver_interface {
    public:
        mongodb_driver(const std::string&);
        ~mongodb_driver();

        const cursor_info& clone(const cursor_request&) override;

        void close(const cursor_request&) override;
        void close_all_cursors(const account_name& code) override;

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

        primary_key_t available_primary_key(const table_info&) override;

        primary_key_t insert(const table_info&, primary_key_t, const variant&) override;
        primary_key_t update(const table_info&, primary_key_t, const variant&) override;
        primary_key_t remove(const table_info&, primary_key_t) override;

    private:
        struct mongodb_impl_;
        std::unique_ptr<mongodb_impl_> impl_;
    }; // class mongodb_driver

} } // namespace cyberway::chaindb
