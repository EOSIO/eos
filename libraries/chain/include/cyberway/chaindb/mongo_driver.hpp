#pragma once

#include <string>
#include <memory>

#include <cyberway/chaindb/driver_interface.hpp>

namespace cyberway { namespace chaindb {

    class mongodb_driver: public driver_interface {
    public:
        mongodb_driver(std::string);
        ~mongodb_driver();

        const cursor_info& clone(cursor_t) override;

        void close(cursor_t) override;
        void close_all_cursors(const account_name& code) override;

        void verify_table_structure(const table_info&, const microseconds&) override;

        const cursor_info& lower_bound(index_info, const variant& key) override;
        const cursor_info& upper_bound(index_info, const variant& key) override;
        const cursor_info& find(index_info, primary_key_t, const variant& key) override;
        const cursor_info& end(index_info) override;

        const cursor_info& current(cursor_t) override;
        const cursor_info& next(cursor_t) override;
        const cursor_info& prev(cursor_t) override;

        const variant& value(const primary_key_t) override;
        const variant& value(const cursor_info&) override;
        const bytes&   data(const cursor_info&) const override;
              bytes&   mutable_data(const cursor_info&) override;

        primary_key_t available_primary_key(const table_info&) override;

        primary_key_t insert(const table_info&, const variant&) override;
        primary_key_t update(const table_info&, const variant&) override;
        primary_key_t remove(const table_info&, primary_key_t) override;

    private:
        struct mongodb_impl_;
        std::unique_ptr<mongodb_impl_> impl_;
    }; // class mongodb_driver

} } // namespace cyberway::chaindb