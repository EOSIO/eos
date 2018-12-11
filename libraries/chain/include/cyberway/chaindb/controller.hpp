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

    using eosio::chain::account_name;
    using eosio::chain::apply_context;
    using eosio::chain::abi_def;

    using table_name_t = eosio::chain::table_name::value_type;
    using index_name_t = eosio::chain::index_name::value_type;
    using account_name_t = account_name::value_type;

    using cursor_t = int32_t;
    static constexpr cursor_t invalid_cursor = (-1);

    struct index_request final {
        const account_name code;
        const account_name scope;
        const table_name_t table;
        const index_name_t index;
    }; // struct index_request

    struct table_request final {
        const account_name code;
        const account_name scope;
        const table_name_t table;
    }; // struct table_request

    struct cursor_request final {
        const account_name code;
        const cursor_t id;
    }; // struct cursor_request

    class abi_info;

    struct table_info {
        const account_name code;
        const account_name scope;
        const table_def*   table    = nullptr;
        const order_def*   pk_order = nullptr;
        const abi_info*    abi      = nullptr;

        table_info(const account_name& code, const account_name& scope)
        : code(code), scope(scope) {
        }
    }; // struct table_info

    struct index_info: public table_info {
        const index_def* index = nullptr;

        using table_info::table_info;
    }; // struct index_info

    class chaindb_controller final {
    public:
        chaindb_controller() = delete;
        chaindb_controller(const chaindb_controller&) = delete;

        chaindb_controller(const microseconds& max_abi_time, chaindb_type, string);
        ~chaindb_controller();

        void drop_db();

        bool has_abi(const account_name&);
        void add_abi(const account_name&, abi_def);
        void remove_abi(const account_name&);

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

        cursor_t lower_bound(const index_request&, const char* key, size_t);
        cursor_t upper_bound(const index_request&, const char* key, size_t);
        cursor_t find(const index_request&, primary_key_t, const char* key, size_t);

        cursor_t end(const index_request&);
        cursor_t clone(const cursor_request&);

        primary_key_t current(const cursor_request&);
        primary_key_t next(const cursor_request&);
        primary_key_t prev(const cursor_request&);

        int32_t       datasize(const cursor_request&);
        primary_key_t data(const cursor_request&, const char*, size_t);

        cache_item_ptr create_cache_item(const table_request&, const cache_converter_interface&);
        cache_item_ptr get_cache_item(const cursor_request&, const table_request&, primary_key_t, const cache_converter_interface&);

        primary_key_t available_pk(const table_request&);

        cursor_t      insert(apply_context&, const table_request&, const account_name&, primary_key_t, const char*, size_t);
        primary_key_t update(apply_context&, const table_request&, const account_name&, primary_key_t, const char*, size_t);
        primary_key_t remove(apply_context&, const table_request&, primary_key_t);

        cursor_t      insert(const table_request&, cache_item_ptr, variant, size_t);
        primary_key_t update(const table_request&, primary_key_t, variant, size_t);
        primary_key_t remove(const table_request&, primary_key_t);

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
