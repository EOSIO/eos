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

    template<class> struct object_to_table;

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
    }; // struct find_info

    struct ram_payer_info final {
        apply_context* ctx   = nullptr; // maybe unavailable - see eosio controller
        account_name   payer;
        size_t         precalc_size = 0;

        ram_payer_info() = default;

        ram_payer_info(apply_context& c, account_name p = account_name(), const size_t s = 0)
        : ctx(&c), payer(std::move(p)), precalc_size(s) {
        }

        void add_usage(const account_name, const int64_t delta) const;
    }; // struct ram_payer_info

    class chaindb_controller final {
    public:
        chaindb_controller() = delete;
        chaindb_controller(const chaindb_controller&) = delete;

        chaindb_controller(chaindb_type, string);
        ~chaindb_controller();

        template<typename Object>
        typename object_to_table<Object>::type get_table() {
            return typename object_to_table<Object>::type(*this);
        }

        template<typename Object>
        const Object* find(const primary_key_t pk) {
            auto midx = get_table<Object>();
            auto itr = midx.find(pk);
            if (midx.end() == itr) {
                return nullptr;
            }
            // should not be critical - object is stored in cache map
            return &*itr;
        }

        template<typename Object>
        const Object* find(const oid<Object>& id = oid<Object>()) {
            return find<Object>(id._id);
        }

        template<typename Object, typename ByIndex, typename Key>
        const Object* find(Key&& key) {
            auto midx = get_table<Object>();
            auto idx = midx.template get_index<ByIndex>();
            auto itr = idx.find(std::forward<Key>(key));
            if (idx.end() == itr) {
                return nullptr;
            }
            // should not be critical - object is stored in cache map
            return &*itr;
        }

        template<typename Object>
        const Object& get(const primary_key_t pk) {
            auto midx = get_table<Object>();
            // should not be critical - object is stored in cache map
            return midx.get(pk);
        }

        template<typename Object>
        const Object& get(const oid<Object>& id = oid<Object>()) {
            return get<Object>(id._id);
        }

        template<typename Object, typename ByIndex, typename Key>
        const Object& get(Key&& key) {
            auto midx = get_table<Object>();
            auto idx = midx.template get_index<ByIndex>();
            // should not be critical - object is stored in cache map
            return idx.get(std::forward<Key>(key));
        }

        template<typename Object, typename Lambda>
        const Object& emplace(Lambda&& constructor) {
            auto midx = get_table<Object>();
            auto itr = midx.emplace(std::forward<Lambda>(constructor));
            // should not be critical - object is stored in cache map
            return *itr;
        }

        template<typename Object, typename Lambda>
        void modify(const Object& obj, Lambda&& updater) {
            auto midx = get_table<Object>();
            midx.modify(obj, std::forward<Lambda>(updater));
        }

        template<typename Object>
        void erase(const Object& obj) {
            auto midx = get_table<Object>();
            midx.erase(obj);
        }

        template<typename Object>
        void erase(const primary_key_t pk) {
            auto midx = get_table<Object>();
            midx.erase(pk);
        }

        template<typename Object>
        void erase(const oid<Object>& id) {
            erase<Object>(id._id);
        }

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

        int64_t insert(const ram_payer_info&, const table_request&, primary_key_t, const char*, size_t);
        int64_t update(const ram_payer_info&, const table_request&, primary_key_t, const char*, size_t);
        int64_t remove(const ram_payer_info&, const table_request&, primary_key_t);

        int64_t insert(cache_item&, variant);
        int64_t update(cache_item&, variant);
        int64_t remove(cache_item&);

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
