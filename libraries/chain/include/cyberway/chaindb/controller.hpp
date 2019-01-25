#pragma once

#include <memory>

#include <fc/variant.hpp>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/cache_item.hpp>

namespace eosio { namespace chain {
    class apply_context;
} } // namespace eosio::chain

namespace cyberway { namespace chaindb {
    using fc::microseconds;
    using fc::variant;

    using eosio::chain::apply_context;
    using eosio::chain::abi_def;

    using table_name_t = eosio::chain::table_name::value_type;
    using index_name_t = eosio::chain::index_name::value_type;
    using account_name_t = account_name::value_type;

    using cursor_t = int32_t;
    static constexpr cursor_t invalid_cursor = (0);

    struct index_request final {
        const account_name code;
        const account_name scope;
        const hash_t       hash;
        const index_name_t index;
    }; // struct index_request

    struct table_request final {
        const account_name code;
        const account_name scope;
        const hash_t       hash;
    }; // struct table_request

    struct cursor_request final {
        const account_name code;
        const cursor_t     id;
    }; // struct cursor_request

    struct index_info: public table_info {
        const index_def* index = nullptr;

        using table_info::table_info;

        index_info(const table_info& src)
        : table_info(src) {
        }
    }; // struct index_info

    struct find_info final {
        cursor_t      cursor = invalid_cursor;
        primary_key_t pk     = end_primary_key;
    };

    class chaindb_controller final {
    public:
        chaindb_controller() = delete;
        chaindb_controller(const chaindb_controller&) = delete;

        chaindb_controller(chaindb_type, string);
        ~chaindb_controller();

        void restore_db();
        void drop_db();

        bool has_abi(const account_name&);
        void add_abi(const account_name&, abi_def);
        void remove_abi(const account_name&);

        const abi_map& get_abi_map() const;

        void close(const cursor_request&);
        void close_code_cursors(const account_name&);

        void apply_all_changes();
        void apply_code_changes(const account_name&);

        chaindb_session start_undo_session(bool enabled);
        void undo();
        void undo_all();
        void commit(int64_t revision);

        int64_t revision() const;
        void set_revision(uint64_t revision);

        find_info lower_bound(const index_request&, const char* key, size_t);
        find_info upper_bound(const index_request&, const char* key, size_t);
        find_info find(const index_request&, primary_key_t, const char* key, size_t);

        find_info begin(const index_request&);
        find_info end(const index_request&);
        find_info clone(const cursor_request&);
        find_info opt_find_by_pk(const table_request& request, primary_key_t pk);

        primary_key_t current(const cursor_request&);
        primary_key_t next(const cursor_request&);
        primary_key_t prev(const cursor_request&);

        int32_t       datasize(const cursor_request&);
        primary_key_t data(const cursor_request&, const char*, size_t);

        void set_cache_converter(const table_request&, const cache_converter_interface&);
        cache_item_ptr create_cache_item(const table_request&);
        cache_item_ptr get_cache_item(const cursor_request&, const table_request&, primary_key_t);

        primary_key_t available_pk(const table_request&);

        primary_key_t insert(apply_context&, const table_request&, const account_name&, primary_key_t, const char*, size_t);
        primary_key_t update(apply_context&, const table_request&, const account_name&, primary_key_t, const char*, size_t);
        primary_key_t remove(apply_context&, const table_request&, primary_key_t);

        primary_key_t insert(cache_item&, variant, size_t);
        primary_key_t update(cache_item&, variant, size_t);
        primary_key_t remove(cache_item&);

        variant value_by_pk(const table_request& request, primary_key_t pk);
        variant value_at_cursor(const cursor_request& request);

    private:
        struct controller_impl_;
        std::unique_ptr<controller_impl_> impl_;
    }; // class chaindb_controller

    class chaindb_guard final {
    public:
        chaindb_guard() = delete;
        chaindb_guard(const chaindb_guard&) = delete;

        chaindb_guard(chaindb_controller& controller, const account_name& code)
        : controller_(controller), code_(code) {
        }

        ~chaindb_guard() {
            controller_.close_code_cursors(code_);
        }

    private:
        chaindb_controller& controller_;
        const account_name& code_;
    }; // class chaindb_guard

} } // namespace cyberway::chaindb
