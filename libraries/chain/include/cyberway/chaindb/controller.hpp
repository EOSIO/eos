#pragma once

#include <memory>

#include <fc/variant.hpp>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/cache_item.hpp>
#include <cyberway/chaindb/storage_payer_info.hpp>

namespace cyberway { namespace chaindb {
    using fc::microseconds;
    using fc::variant;

    using eosio::chain::abi_def;

    template<class> struct object_to_table;
    struct storage_payer_info;

    class chaindb_controller final {
    public:
        chaindb_controller() = delete;
        chaindb_controller(const chaindb_controller&) = delete;

        chaindb_controller(chaindb_type, string, string);
        ~chaindb_controller();

        template<typename Object>
        auto get_table() const {
            return typename object_to_table<Object>::type(*this);
        }

        template<typename Object, typename Index>
        auto get_index() const {
            return get_table<Object>().template get_index<Index>();
        }

        template<typename Object>
        const Object* find(const primary_key_t pk) const {
            auto midx = get_table<Object>();
            auto itr = midx.find(pk);
            if (midx.end() == itr) {
                return nullptr;
            }
            // should not be critical - object is stored in cache map
            return &*itr;
        }

        template<typename Object>
        const Object* find(const oid<Object>& id = oid<Object>()) const {
            return find<Object>(id._id);
        }

        template<typename Object, typename ByIndex, typename Key>
        const Object* find(Key&& key) const {
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
        const Object& get(const primary_key_t pk) const {
            auto midx = get_table<Object>();
            // should not be critical - object is stored in cache map
            return midx.get(pk);
        }

        template<typename Object>
        const Object& get(const oid<Object>& id = oid<Object>()) const {
            return get<Object>(id._id);
        }

        template<typename Object, typename ByIndex, typename Key>
        const Object& get(Key&& key) const {
            auto midx = get_table<Object>();
            auto idx = midx.template get_index<ByIndex>();
            // should not be critical - object is stored in cache map
            return idx.get(std::forward<Key>(key));
        }

        template<typename Object, typename Lambda>
        const Object& emplace(Lambda&& constructor) const {
            return emplace<Object>({}, std::forward<Lambda>(constructor));
        }

        template<typename Object, typename Lambda>
        const Object& emplace(const storage_payer_info& payer, Lambda&& constructor) const {
            auto midx = get_table<Object>();
            auto res = midx.emplace(payer, std::forward<Lambda>(constructor));
            // should not be critical - object is stored in cache map
            return res.obj;
        }

        template<typename Object, typename Lambda>
        int64_t modify(const Object& obj, Lambda&& updater) const {
            auto midx = get_table<Object>();
            return midx.modify(obj, std::forward<Lambda>(updater));
        }

        template<typename Object, typename Lambda>
        int64_t modify(const Object& obj, const storage_payer_info& payer, Lambda&& updater) const {
            auto midx = get_table<Object>();
            return midx.modify(obj, payer, std::forward<Lambda>(updater));
        }

        template<typename Object>
        int64_t erase(const Object& obj, const storage_payer_info& payer = {}) const {
            auto midx = get_table<Object>();
            return midx.erase(obj, payer);
        }

        template<typename Object>
        int64_t erase(const primary_key_t pk, const storage_payer_info& payer = {}) const {
            auto midx = get_table<Object>();
            return midx.erase(midx.get(pk), payer);
        }

        template<typename Object>
        int64_t erase(const oid<Object>& id, const storage_payer_info& payer = {}) const {
            return erase<Object>(id._id, payer);
        }

        void restore_db() const;
        void drop_db() const;
        void clear_cache() const;

        bool has_abi(const account_name&) const;
        void add_abi(const account_name&, abi_def) const;
        void remove_abi(const account_name&) const;

        const abi_map& get_abi_map() const;

        void close(const cursor_request&) const;
        void close_code_cursors(const account_name&) const;

        void apply_all_changes() const;
        void apply_code_changes(const account_name&) const;

        revision_t revision() const;
        void set_revision(revision_t revision) const;

        chaindb_session start_undo_session(bool enabled) const;
        void undo_last_revision() const;
        void commit_revision(revision_t) const;

        find_info lower_bound(const index_request&, const char* key, size_t) const;
        find_info lower_bound(const table_request&, primary_key_t) const;
        find_info lower_bound(const index_request& request, const variant&) const;

        find_info upper_bound(const index_request&, const char* key, size_t) const;
        find_info upper_bound(const table_request&, primary_key_t) const;
        find_info upper_bound(const index_request& request, const variant&) const;

        find_info locate_to(const index_request&, const char* key, size_t, primary_key_t) const;

        find_info begin(const index_request&) const;
        find_info end(const index_request&) const;
        find_info clone(const cursor_request&) const;

        primary_key_t current(const cursor_request&) const;
        primary_key_t next(const cursor_request&) const;
        primary_key_t prev(const cursor_request&) const;

        void set_cache_converter(const table_request&, const cache_converter_interface&) const;
        cache_object_ptr create_cache_object(const table_request&) const;
        cache_object_ptr get_cache_object(const cursor_request&, bool with_blob) const;

        primary_key_t available_pk(const table_request&) const;

        int insert(const table_request&, const storage_payer_info&, primary_key_t, const char*, size_t) const;
        int update(const table_request&, const storage_payer_info&, primary_key_t, const char*, size_t) const;
        int remove(const table_request&, const storage_payer_info&, primary_key_t) const;

        int insert(const table_request&, primary_key_t, variant, const storage_payer_info&) const;

        int insert(cache_object&, variant, const storage_payer_info&) const;
        int update(cache_object&, variant, const storage_payer_info&) const;
        int remove(cache_object&, const storage_payer_info&) const;

        void recalc_ram_usage(cache_object&, const storage_payer_info&) const;

        variant value_by_pk(const table_request& request, primary_key_t pk) const;
        variant value_at_cursor(const cursor_request& request) const;
        object_value object_at_cursor(const cursor_request& request) const;

    private:
        friend class chaindb_session;

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
        const chaindb_controller& controller_;
        const account_name& code_;
    }; // class chaindb_guard

} } // namespace cyberway::chaindb
