#include <cyberway/chaindb/mongo_driver.hpp>
#include <cyberway/chaindb/exception.hpp>

#define NOT_SUPPORTED CYBERWAY_ASSERT(false, broken_driver_exception, "MongoDB driver is not supported")

namespace cyberway { namespace chaindb {
    struct mongodb_driver::mongodb_impl_ {
    }; // struct mongodb_driver::mongodb_impl_

    mongodb_driver::mongodb_driver(string, string) {
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

    const cursor_info& mongodb_driver::clone(const cursor_request&) {
        NOT_SUPPORTED;
    }

    void mongodb_driver::close(const cursor_request&) {
        NOT_SUPPORTED;
    }

    void mongodb_driver::close_all_cursors(const account_name&) {
        NOT_SUPPORTED;
    }

    void mongodb_driver::apply_changes() {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::lower_bound(index_info, variant) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::upper_bound(index_info, variant) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::find(index_info, primary_key_t, variant) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::begin(index_info) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::end(index_info) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::opt_find_by_pk(index_info, primary_key_t) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::current(const cursor_info&) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::current(const cursor_request&) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::next(const cursor_request&) {
        NOT_SUPPORTED;
    }

    const cursor_info& mongodb_driver::prev(const cursor_request&) {
        NOT_SUPPORTED;
    }

    variant mongodb_driver::object_by_pk(const table_info&, const primary_key_t) {
        NOT_SUPPORTED;
    }

    const variant& mongodb_driver::object_at_cursor(const cursor_info&) {
        NOT_SUPPORTED;
    }

    void mongodb_driver::set_blob(const cursor_info&, bytes) {
        NOT_SUPPORTED;
    }

    primary_key_t mongodb_driver::available_primary_key(const table_info&) {
        NOT_SUPPORTED;
    }

    void write(const table_info&, primary_key_t, revision_t, write_value, write_value) {
        NOT_SUPPORTED;
    }

} } // namespace cyberway::chaindb