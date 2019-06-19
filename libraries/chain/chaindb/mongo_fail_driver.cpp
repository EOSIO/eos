#include <cyberway/chaindb/mongo_driver.hpp>
#include <cyberway/chaindb/exception.hpp>

#define NOT_SUPPORTED CYBERWAY_THROW(broken_driver_exception, "MongoDB driver is not supported")

namespace cyberway { namespace chaindb {
    struct mongodb_driver::mongodb_impl_ {
    }; // struct mongodb_driver::mongodb_impl_

    mongodb_driver::mongodb_driver(journal&, string, string) {
        NOT_SUPPORTED;
    }

    mongodb_driver::~mongodb_driver() = default;

    std::vector<table_def> mongodb_driver::db_tables(const account_name&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::create_index(const index_info&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::drop_index(const index_info&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::drop_table(const table_info&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::drop_db() const {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::clone(const cursor_request&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::close(const cursor_request&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::close_code_cursors(const account_name&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::apply_code_changes(const account_name&) const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::apply_all_changes() const {
        NOT_SUPPORTED;
    }

    void mongodb_driver::skip_pk(const table_info&, primary_key_t) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::lower_bound(index_info, variant) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::upper_bound(index_info, variant) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::locate_to(index_info, variant, primary_key_t) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::begin(index_info) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::end(index_info) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::cursor(const cursor_request&) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::current(const cursor_info&) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::next(const cursor_info&) const {
        NOT_SUPPORTED;
    }

    cursor_info& mongodb_driver::prev(const cursor_info&) const {
        NOT_SUPPORTED;
    }

    primary_key_t mongodb_driver::available_pk(const table_info&) const {
        NOT_SUPPORTED;
    }

    object_value mongodb_driver::object_by_pk(const table_info&, const primary_key_t) const {
        NOT_SUPPORTED;
    }

    const object_value& mongodb_driver::object_at_cursor(const cursor_info&, bool ) const {
        NOT_SUPPORTED;
    }

} } // namespace cyberway::chaindb