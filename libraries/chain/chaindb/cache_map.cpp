#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/abi_info.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/table_info.hpp>

#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>

/** Cache exception is a critical errors and it doesn't handle by chain */
#define CYBERWAY_CACHE_ASSERT(expr, FORMAT, ...)                      \
    FC_MULTILINE_MACRO_BEGIN                                          \
        if (BOOST_UNLIKELY( !(expr) )) {                              \
            elog(FORMAT, __VA_ARGS__);                                \
            FC_THROW_EXCEPTION(cache_exception, FORMAT, __VA_ARGS__); \
        }                                                             \
    FC_MULTILINE_MACRO_END

namespace cyberway { namespace chaindb {

    namespace intr = boost::intrusive;

    struct cache_object_compare {
        const service_state& service(const service_state& v) const { return v;           }
        const service_state& service(const cache_object&  v) const { return v.service(); }

        template<typename LeftKey, typename RightKey>
        bool operator()(const LeftKey& l, const RightKey& r) const {
            return operator()(service(l), service(r));
        }

        bool operator()(const service_state& l, const service_state& r) const {
            if (l.pk    < r.pk   ) return true;
            if (l.pk    > r.pk   ) return false;

            if (l.table < r.table) return true;
            if (l.table > r.table) return false;

            if (l.code  < r.code ) return true;
            if (l.code  > r.code ) return false;

            return l.scope < r.scope;
        }
    }; // struct cache_object_compare

    bool operator<(const cache_object& l, const cache_object& r) {
        return cache_object_compare()(l, r);
    }

    //---------------------------------------

    struct cache_service_key final {
        account_name code;
        table_name   table;

        cache_service_key(const object_value& obj)
        : code(obj.service.code), table(obj.service.table) {
        }

        cache_service_key(const table_info& tab)
        : code(tab.code), table(tab.table_name()) {
        }
    }; // cache_service_key

    bool operator<(const cache_service_key& l, const cache_service_key& r) {
        if (l.code < r.code) return true;
        if (l.code > r.code) return false;
        return l.table < r.table;
    }

    //---------------------------------------

    struct cache_service_info final {
        int cache_object_cnt = 0;
        primary_key_t next_pk = primary_key::Unset;
        const cache_converter_interface* converter = nullptr; // exist only for interchain tables

        cache_service_info() = default;

        cache_service_info(const cache_converter_interface& c)
        : converter(&c) {
        }

        bool empty() const {
            assert(cache_object_cnt >= 0);
            return (!converter /* info for contacts tables */ && !cache_object_cnt);
        }
    }; // struct cache_service_info

    //---------------------------------------

    cache_index_value::cache_index_value(index_name i, bytes b, const cache_object& c)
    : index(std::move(i)),
      blob(std::move(b)),
      object(&c) {
    }

    struct cache_index_key final {
        const service_state& service;
        const index_name_t   index;
        const char*          blob;
        const size_t         size;
    }; // struct cache_index_key

    struct cache_index_compare {
        index_name_t   index(const cache_index_value& v) const { return v.index;                   }
        index_name_t   index(const cache_index_key& v  ) const { return v.index;                   }

        table_name_t   table(const cache_index_value& v) const { return v.object->service().table; }
        table_name_t   table(const cache_index_key& v  ) const { return v.service.table;           }

        account_name_t code (const cache_index_value& v) const { return v.object->service().code;  }
        account_name_t code (const cache_index_key& v  ) const { return v.service.code;            }

        scope_name_t   scope(const cache_index_value& v) const { return v.object->service().scope; }
        scope_name_t   scope(const cache_index_key& v  ) const { return v.service.scope;           }

        size_t         size (const cache_index_value& v) const { return v.blob.size();             }
        size_t         size (const cache_index_key& v  ) const { return v.size;                    }

        const char*    data (const cache_index_value& v) const { return v.blob.data();             }
        const char*    data (const cache_index_key& v  ) const { return v.blob;                    }

        template<typename LeftKey, typename RightKey>
        bool operator()(const LeftKey& l, const RightKey& r) const {
            if (index(l) < index(r)) return true;
            if (index(l) > index(r)) return false;

            if (table(l) < table(r)) return true;
            if (table(l) > table(r)) return false;

            if (code(l)  < code(r) ) return true;
            if (code(l)  > code(r) ) return false;

            if (scope(l) < scope(r)) return true;
            if (scope(l) > scope(r)) return false;

            if (size(l)  < size(r) ) return true;
            if (size(l)  > size(r) ) return false;

            return (std::memcmp(data(l), data(r), size(l)) < 0);
        }

    }; // struct cache_index_compare

    bool operator<(const cache_index_value& l, const cache_index_value& r) {
        return cache_index_compare()(l, r);
    }

    //---------------------------------------

    struct pending_cache_object_state final: public cache_object_state {
        using cache_object_state::cache_object_state;

        ~pending_cache_object_state() {
            reset();
        }

        static pending_cache_object_state* cast(cache_object_state* state) {
            if (state && state->cell && cache_cell::Pending == state->cell->kind) {
                return static_cast<pending_cache_object_state*>(state);
            }
            return nullptr;
        }

        static pending_cache_object_state& cast(cache_object_state& state) {
            auto pending_ptr = cast(&state);
            assert(pending_ptr);
            return *pending_ptr;
        }

        void reset() {
            if (!object_ptr) {
                return;
            }

            auto tmp_obj_ptr = std::move(object_ptr);

            auto tmp_prev_state = prev_state;
            prev_state = nullptr;

            if (!tmp_obj_ptr->is_same_cell(*cell)) {
                return;
            }

            if (!tmp_prev_state) {
                object_ptr->state_ = nullptr;
                cell->map->release_cache_object(*object_ptr, is_deleted);
                return;
            }

            tmp_obj_ptr->swap_state(*tmp_prev_state);

            auto pending_prev_state = cast(tmp_prev_state);
            if (pending_prev_state) {
                pending_prev_state->is_deleted = is_deleted;
            }
        }

        void commit() {
            if (!object_ptr) {
                return;
            }

            if (prev_state) {
                prev_state->object_ptr.reset();
            }
            prev_state = nullptr;

            if (is_deleted) {
                object_ptr->state_ = nullptr;
                cell->map->release_cache_object(*object_ptr, is_deleted);
            }
        }

        cache_object_state* prev_state = nullptr;
        bool is_deleted = false;
    }; // struct pending_cache_object_state

    struct pending_cache_cell final: public cache_cell {
        pending_cache_cell(cache_map_impl& m, const revision_t r, const uint64_t d)
        : cache_cell(m, cache_cell::Pending),
          revision_(r),
          max_distance_(d) {
            assert(revision_ >= start_revision);
            assert(max_distance_ > 0);
        }

        void emplace(cache_object_ptr obj_ptr, const bool is_deleted = false, const bool copy_prev = false) {
            assert(obj_ptr);
            assert(!copy_prev || obj_ptr->cell().kind == cache_cell::Pending);

            state_list.emplace_back(*this, std::move(obj_ptr));

            auto& state = state_list.back();
            auto  prev_state = state.object_ptr->swap_state(state);

            if (copy_prev) {
                auto& pending_prev_state = pending_cache_object_state::cast(*prev_state);
                state.prev_state = pending_prev_state.prev_state;
                state.is_deleted = pending_prev_state.is_deleted;
            } else {
                state.prev_state = prev_state;
                state.is_deleted = is_deleted;
                add_ram_usage(state);
            }
        }

        void squash_revision() {
            revision_--;
            assert(revision_ >= start_revision);
        }

        revision_t revision() {
            return revision_;
        }

        std::deque<pending_cache_object_state> state_list; // Expansion of a std::deque is cheaper than the expansion of a std::vector

    private:
        void add_ram_usage(const pending_cache_object_state& state) {
            auto object_size = state.object_ptr->service().size;
            if (!object_size) {
                return;
            }
    
            auto delta = state.prev_state ?
                std::max(eosio::chain::int_arithmetic::safe_prop<uint64_t>(object_size, pos - state.prev_state->cell->pos, max_distance_), uint64_t(1)) : 
                object_size * config::ram_load_multiplier;
                
            CYBERWAY_CACHE_ASSERT(UINT64_MAX - size >= delta, "Pending delta would overflow UINT64_MAX");
            size += delta;

        }

        revision_t revision_ = impossible_revision;
        const uint64_t max_distance_ = 0;
    }; // struct pending_cache_cell

    //---------------------------------------

    struct lru_cache_object_state final: public cache_object_state {
        using cache_object_state::cache_object_state;

        ~lru_cache_object_state() {
            reset();
        }

        static lru_cache_object_state* cast(cache_object_state* state) {
            if (state && state->cell && cache_cell::LRU == state->cell->kind) {
                return static_cast<lru_cache_object_state*>(state);
            }
            return nullptr;
        }

        static lru_cache_object_state& cast(cache_object_state& state) {
            auto lru_ptr = cast(&state);
            assert(lru_ptr);
            return *lru_ptr;
        }

        void reset() {
            if (!object_ptr) {
                return;
            }

            if (object_ptr->has_cell()) {
                object_ptr->state_ = nullptr;
                cell->map->release_cache_object(*object_ptr, false);
            }
            object_ptr.reset();
        }
    }; // struct lru_cache_object_state

    struct lru_cache_cell final: public cache_cell {
        lru_cache_cell(cache_map_impl& m, const uint64_t p)
        : cache_cell(m, cache_cell::LRU) {
            pos = p;
        }

        void emplace(cache_object_ptr obj_ptr) {
            state_list.emplace_back(*this, std::move(obj_ptr));

            auto& state = state_list.back();
            auto* prev_state = state.object_ptr->swap_state(state);

            if (prev_state) {
                prev_state->reset();
            }
        }

        std::deque<lru_cache_object_state> state_list; // Expansion of a std::deque is cheaper than the expansion of a std::vector
    }; // struct lru_cache_cell

    //---------------------------------------

    struct system_cache_object_state final: public cache_object_state {
        using cache_object_state::cache_object_state;

        ~system_cache_object_state() {
            reset();
        }

        static system_cache_object_state* cast(cache_object_state* state) {
            if (state && state->cell && cache_cell::System == state->cell->kind) {
                return static_cast<system_cache_object_state*>(state);
            }
            return nullptr;
        }

        static system_cache_object_state& cast(cache_object_state& state) {
            auto* system_ptr = cast(&state);
            assert(system_ptr);
            return *system_ptr;
        }

        void reset();

        std::list<system_cache_object_state>::iterator itr;
    }; // struct pending_cache_object_state

    struct system_cache_cell final: public cache_cell {
        system_cache_cell(cache_map_impl& m)
        : cache_cell(m, cache_cell::System) {
        }

        ~system_cache_cell() {
            clear();
        }

        void clear() {
            for (auto& state: state_list) if (state.object_ptr) {
                assert(state.object_ptr->is_same_cell(*this));
                state.itr = state_list.end();
                map->remove_cache_object(*state.object_ptr);
            }

            state_list.clear();
        }

        void emplace(cache_object_ptr obj_ptr) {
            size += obj_ptr->service().size;

            state_list.emplace_back(*this, std::move(obj_ptr));

            auto& state = state_list.back();
            auto* prev_state = state.object_ptr->swap_state(state);
            assert(!prev_state);

            state.itr = --state_list.end();
        }

        std::list<system_cache_object_state> state_list;
    }; // struct system_cache_cell

    void system_cache_object_state::reset() {
        if (!object_ptr) {
            return;
        }

        assert(!object_ptr->has_cell());

        auto system_cell = static_cast<system_cache_cell*>(cell);
        auto system_itr  = itr;

        object_ptr.reset();

        if (system_cell->state_list.end() != system_itr) {
            itr = system_cell->state_list.end();
            system_cell->state_list.erase(system_itr);
        }
    }

    //---------------------------------------

    void cache_object_state::reset() {
        switch (cell->kind) {
            case cache_cell::System:
                system_cache_object_state::cast(*this).reset();
                break;

            case cache_cell::Pending:
                pending_cache_object_state::cast(*this).reset();
                break;

            case cache_cell::LRU: {
                lru_cache_object_state::cast(*this).reset();
                break;
            }

            case cache_cell::Unknown:
            default:
                assert(false);
        }
    }

    //---------------------------------------

    class cache_map_impl final {
    public:
        cache_map_impl()
        : system_cell_(*this) {
        }

        ~cache_map_impl() {
            system_cell_.clear();
            clear();
        }

        cache_object_ptr find(const service_state& key) {
            auto obj_ptr = find_in_cache(key);
            if (obj_ptr) {
                add_pending_object(obj_ptr);
            }
            return obj_ptr;
        }

        cache_object_ptr find(const cache_index_key& key) {
            cache_object_ptr obj_ptr;
            auto itr = cache_index_tree_.find(key, cache_index_compare());
            if (cache_index_tree_.end() != itr) {
                obj_ptr = cache_object_ptr(const_cast<cache_object*>(itr->object));
                add_pending_object(obj_ptr);
            }
            return obj_ptr;
        }

        cache_object_ptr emplace(const table_info& table, object_value value) {
            if (!value.service.in_ram) {
                return create_cache_object(table, std::move(value));
            } else {
                return emplace_in_ram(table, std::move(value));
            }
        }

        void remove_cache_object(const service_state& key) {
            auto obj_ptr = find(key);
            if (obj_ptr) {
                remove_cache_object(*obj_ptr);
            }
        }

        void remove_cache_object(cache_object& obj) {
            assert(!obj.is_deleted());

            auto is_released = delete_cache_object(obj);
            if (is_released) {
                auto& tmp_state = *obj.state_;
                obj.state_ = nullptr;
                tmp_state.reset();
            }
        }

        void set_object(const table_info& table, cache_object& cache_obj, object_value value) {
            assert(cache_obj.service().empty() || cache_obj.is_valid_table(value.service));

            int delta = value.service.size - cache_obj.service().size;
            cache_obj.object_ = std::move(value);
            cache_obj.blob_.clear();

            if (!cache_obj.has_cell()) {
                return;
            }

            add_ram_usage(cache_obj.cell(), delta);

            if (is_system_code(cache_obj.service().code)) {
                using state_object = eosio::chain::resource_limits::resource_limits_state_object;
                if (cache_obj.has_data() && cache_obj.service().table == chaindb::tag<state_object>::get_code()) {
                    auto& state = multi_index_item_data<state_object>::get_T(cache_obj);
                    ram_limit_ = get_ram_limit(state.ram_size, state.reserved_ram_size);
                }

                // don't rebuild indicies for interchain objects
                if (cache_obj.indicies_.capacity()) {
                    return;
                }
            }

            delete_cache_indicies(cache_obj.indicies_);
            build_cache_indicies(table, cache_obj);
        }

        void set_service(const table_info&, cache_object& cache_obj, service_state service) {
            assert(cache_obj.is_valid_table(service));

            cache_obj.object_.service = std::move(service);
            if (!cache_obj.service().in_ram && cache_obj.has_cell()) {
                remove_cache_object(cache_obj);
            }
        }

        void set_revision(const service_state& key, const revision_t rev) {
            auto obj_ptr = find(key);
            if (obj_ptr) {
                obj_ptr->object_.service.revision = rev;
            }
        }

        primary_key_t get_next_pk(const table_info& table) {
            auto service_ptr = find_cache_service(table);
            if (service_ptr && primary_key::Unset != service_ptr->next_pk) {
                return service_ptr->next_pk++;
            }
            return primary_key::Unset;
        }

        void set_next_pk(const table_info& table, const primary_key_t pk) {
            auto service_ptr = find_cache_service(table);
            if (service_ptr) {
                service_ptr->next_pk = pk;
            }
        }

        void set_cache_converter(cache_service_key key, const cache_converter_interface& converter) {
            auto res = service_tree_.emplace(std::move(key), cache_service_info(converter));
            assert(res.second);
        }

        void clear() {
            CYBERWAY_CACHE_ASSERT(!has_pending_cell(), "Clearing of cache with pending changes");

            lru_cell_list_.clear();

            for (auto& service: service_tree_) {
                service.second.next_pk = primary_key::Unset;
            }
        }

        uint64_t calc_ram_bytes(const revision_t revision) {
            return get_pending_cell(revision).size;
        }

        void set_revision(const revision_t revision) {
            if (lru_cell_list_.empty() || !lru_cell_list_.back().state_list.empty()) {
                uint64_t pos = 0;
                if (!lru_cell_list_.empty()) {
                    pos = lru_cell_list_.back().pos;
                }
                lru_cell_list_.emplace_back(*this, pos);
            }
            lru_revision_ = revision;
        }

        void start_session(const revision_t revision) {
            pending_cell_list_.emplace_back(*this, revision, ram_limit_);
            auto& pending = pending_cell_list_.back();
            if (!lru_cell_list_.empty()) {
                auto& lru = lru_cell_list_.back();
                pending.pos = lru.pos + lru.size;
            }
        }

        void push_session(const revision_t revision) {
            CYBERWAY_CACHE_ASSERT(!pending_cell_list_.empty(), "Push session for empty pending caches");
            auto& pending = get_pending_cell(revision);

            set_revision(revision - 1);
            auto& lru = lru_cell_list_.back();
            lru.pos = pending.pos;

            for (auto& state: pending.state_list) {
                state.commit();
                if (!state.is_deleted) {
                    add_ram_usage(lru, state.object_ptr->service().size);
                    lru.emplace(std::move(state.object_ptr));
                }
            }
            pending_cell_list_.pop_back();
            CYBERWAY_CACHE_ASSERT(pending_cell_list_.empty(), "After pushing session still exist pending caches");
            CYBERWAY_CACHE_ASSERT(deleted_object_tree_.empty(), "Tree with deleted object still has items");

            clear_overused_ram();
        }

        void squash_session(const revision_t revision) {
            CYBERWAY_CACHE_ASSERT(!pending_cell_list_.empty(), "Squash session for empty pending caches");

            auto itr = pending_cell_list_.end();
            --itr; auto& src_cell =*itr;
            CYBERWAY_CACHE_ASSERT(revision == src_cell.revision(), "Wrong revision of top pending caches");

            // replay and squashing system transaction to LRU
            if (pending_cell_list_.begin() == itr) {
                CYBERWAY_CACHE_ASSERT(revision - 1 == lru_revision_,
                    "Wrong revision on squashing of sys-transaction to replayed block");
                return push_session(revision);
            }

            --itr; auto& dst_cell =*itr;
            CYBERWAY_CACHE_ASSERT(revision - 1 == dst_cell.revision(), "Wrong revision of pending caches");
            
            if (pending_cell_list_.begin() == itr && dst_cell.state_list.empty()) {
                src_cell.squash_revision();
                pending_cell_list_.pop_front();
                return;
            } 
                
            for (auto& src_state: src_cell.state_list) {
                if (!src_state.prev_state && src_state.is_deleted) {
                    // undo it
                } else if (!src_state.prev_state || src_state.prev_state->cell != &dst_cell) {
                    dst_cell.emplace(std::move(src_state.object_ptr), false, true);
                }
            }

            pending_cell_list_.pop_back();
        }

        void undo_session(const revision_t revision) {
            if (!has_pending_cell()) {
                // case of pop_block
                return;
            }

            auto& pending = get_pending_cell(revision);
            pending_cell_list_.pop_back();
            CYBERWAY_CACHE_ASSERT(!pending_cell_list_.empty() || deleted_object_tree_.empty(),
                "Tree with deleted object still has items");
        }

        void release_cache_object(cache_object& cache_obj, const bool is_deleted) {
            if (is_deleted) {
                auto itr = deleted_object_tree_.iterator_to(cache_obj);
                deleted_object_tree_.erase(itr);
            } else {
                auto tree_itr = cache_object_tree_.iterator_to(cache_obj);
                cache_object_tree_.erase(tree_itr);
                delete_cache_indicies(cache_obj.indicies_);

                auto service_itr = service_tree_.find(cache_obj.object());
                CYBERWAY_CACHE_ASSERT(service_tree_.end() != service_itr, "No service info on release object");

                service_itr->second.cache_object_cnt--;
                if (service_itr->second.empty()) {
                    service_tree_.erase(service_itr);
                }

                if (cache_obj.has_cell()) {
                    add_ram_usage(cache_obj.cell(), -cache_obj.service().size);
                }
            }
        }

        bool delete_cache_object(cache_object& obj) {
            release_cache_object(obj, false);

            if (!has_pending_cell() || obj.cell().kind == cache_cell::System) {
                return true;
            }

            add_pending_object(cache_object_ptr(&obj), true);
            deleted_object_tree_.insert(obj); // to save position in LRU
            return false;
        }

    private:
        using cache_object_tree_type = intr::set<cache_object, intr::constant_time_size<false>>;
        using cache_index_tree_type  = intr::set<cache_index_value, intr::constant_time_size<false>>;
        using lru_cell_list_type     = std::deque<lru_cache_cell>;
        using pending_cell_list_type = std::deque<pending_cache_cell>;
        using service_tree_type      = fc::flat_map<cache_service_key, cache_service_info>;

        cache_object_tree_type cache_object_tree_;
        cache_object_tree_type deleted_object_tree_;
        cache_index_tree_type  cache_index_tree_;
        service_tree_type      service_tree_;

        pending_cell_list_type pending_cell_list_;
        lru_cell_list_type     lru_cell_list_;
        revision_t             lru_revision_ = impossible_revision;
        system_cache_cell      system_cell_;

        uint64_t               ram_limit_ = get_ram_limit();
        uint64_t               ram_used_  = 0;

        static uint64_t get_ram_limit(
            const uint64_t limit   = config::default_ram_size,
            const uint64_t reserve = config::default_reserved_ram_size
        ) {
            // reserve for system objects (transactions, blocks, ...)
            //     and for pending caches (pending_cell_list_, ...)
            return limit - reserve;
        }

        void clear_overused_ram() {
            while (ram_limit_ < ram_used_) {
                assert(!lru_cell_list_.empty());
                auto& lru = lru_cell_list_.front();
                add_ram_usage(lru, -lru.size);
                lru_cell_list_.pop_front();
            }
        }

        void add_system_object(cache_object_ptr obj_ptr) {
            assert(obj_ptr && !obj_ptr->service().payer);
            system_cell_.emplace(std::move(obj_ptr));
        }

        void add_lru_object(cache_object_ptr obj_ptr) {
            assert(obj_ptr);
            if (lru_cell_list_.empty()) {
                set_revision(-1);
            }
            auto& lru = lru_cell_list_.back();
            add_ram_usage(lru, obj_ptr->service().size);
            lru.emplace(std::move(obj_ptr));
        }

        void add_pending_object(cache_object_ptr obj_ptr) {
            assert(obj_ptr);

            auto pending_ptr = find_pending_cell();
            if (!pending_ptr) {
                return;
            }

            if (!obj_ptr->has_cell() || cache_cell::LRU == obj_ptr->cell().kind) {
                pending_ptr->emplace(std::move(obj_ptr), false, false);
            }
        }

        void add_pending_object(cache_object_ptr obj_ptr, const bool is_deleted) {
            assert(obj_ptr);

            auto pending_ptr = find_pending_cell();
            if (!pending_ptr) {
                return;
            }

            if (obj_ptr->has_cell()) switch(obj_ptr->cell().kind) {
                case cache_cell::System:
                    return;

                case cache_cell::Pending: {
                    auto& state = pending_cache_object_state::cast(obj_ptr->state());
                    if (state.is_deleted == is_deleted) {
                        return;
                    }
                    if (state.cell == pending_ptr) {
                        // the same cell -> only change state
                        state.is_deleted = is_deleted;
                        return;
                    }
                    break;
                }

                case cache_cell::LRU:
                    break;

                case cache_cell::Unknown:
                default:
                    assert(false);
            }

            pending_ptr->emplace(std::move(obj_ptr), is_deleted, false);
        }

        pending_cache_cell* find_pending_cell() {
            if (pending_cell_list_.empty()) {
                return nullptr;
            }
            return &pending_cell_list_.back();
        }

        bool has_pending_cell() const {
            return !pending_cell_list_.empty();
        }

        pending_cache_cell& get_pending_cell(const revision_t revision) {
            auto pending_ptr = find_pending_cell();
            CYBERWAY_CACHE_ASSERT(pending_ptr && pending_ptr->revision() == revision,
                "Wrong pending revision ${pending_revision} != ${revision}",
                ("pending_revision", pending_ptr ? pending_ptr->revision() : impossible_revision)("revision", revision));
            return *pending_ptr;
        }

        template <typename Table> cache_service_info& get_cache_service(const Table& table) {
            return service_tree_.emplace(cache_service_key(table), cache_service_info()).first->second;
        }

        template <typename Table> cache_service_info* find_cache_service(const Table& table) {
            auto itr = service_tree_.find(cache_service_key(table));
            if (service_tree_.end() != itr) {
                return &itr->second;
            }
            return nullptr;
        }

        void build_cache_indicies(const table_info& table, cache_object& cache_obj) {
            if (cache_obj.object_.is_null()) {
                return;
            }

            auto& object   = cache_obj.object_;
            auto& service  = object.service;
            auto& indicies = cache_obj.indicies_;
            indicies.reserve(table.table->indexes.size() - 1 /* skip primary */);

            auto index = index_info(service.code, service.scope);
            index.table = table.table;

            for (auto& idx: table.table->indexes) if (idx.unique && idx.name != names::primary_index) {
                index.index = &idx;

                auto blob = table.abi().to_bytes(index, object.value);
                if (blob.empty()) {
                    continue;
                }

                indicies.emplace_back(idx.name, std::move(blob), cache_obj);
                CYBERWAY_ASSERT(cache_index_tree_.end() == cache_index_tree_.find(indicies.back()),
                    driver_duplicate_exception, "Cache duplicate unique records in the index ${index}",
                    ("index", get_full_index_name(index)));
            }

            for (auto& idx: indicies) {
                cache_index_tree_.insert(idx);
            }
        }

        void delete_cache_indicies(cache_indicies& indicies) {
            for (auto& cache: indicies) {
                auto itr = cache_index_tree_.iterator_to(cache);
                cache_index_tree_.erase(itr);
            }
            indicies.clear();
        }

        cache_object_ptr find_in_cache(const service_state& key) {
            cache_object_ptr obj_ptr;
            auto itr = cache_object_tree_.find(key, cache_object_compare());
            if (cache_object_tree_.end() != itr) {
                obj_ptr = cache_object_ptr(&*itr);
            }
            return obj_ptr;
        }

        cache_object_ptr find_in_deleted(const service_state& key) {
            cache_object_ptr obj_ptr;
            auto itr = deleted_object_tree_.find(key, cache_object_compare());
            if (deleted_object_tree_.end() != itr) {
                obj_ptr = cache_object_ptr(&*itr);
            }
            return obj_ptr;
        }

        cache_object_ptr emplace_in_ram(const table_info& table, object_value value) {
            auto cache_obj_ptr = find_in_cache(value.service);
            bool is_new_ptr = false;
            bool is_del_ptr = false;

            if (!cache_obj_ptr && has_pending_cell()) {
                cache_obj_ptr = find_in_deleted(value.service);
                is_del_ptr = !!cache_obj_ptr;
            }

            if (!cache_obj_ptr) {
                object_value obj;
                obj.service = value.service;
                obj.service.size = 0;
                cache_obj_ptr = cache_object_ptr(new cache_object(std::move(obj)));
                is_new_ptr = true;
            }

            assert(!is_new_ptr || !is_del_ptr);

            auto& cache_obj = *cache_obj_ptr;
            auto& service = get_cache_service(value);
            convert_value(&service, cache_obj, std::move(value));

            if (is_del_ptr) {
                auto itr = deleted_object_tree_.iterator_to(cache_obj);
                deleted_object_tree_.erase(itr);
                add_pending_object(cache_obj_ptr, false);
            } else if (is_new_ptr) {
                if (!value.service.payer) {
                    add_system_object(cache_obj_ptr);
                } else if (has_pending_cell()) {
                    add_pending_object(cache_obj_ptr);
                } else {
                    add_lru_object(cache_obj_ptr);
                }
            }

            if (is_new_ptr || is_del_ptr) {
                cache_object_tree_.insert(cache_obj);
                service.cache_object_cnt++;
            }

            set_object(table, cache_obj, std::move(value));

            return cache_obj_ptr;
        }

        cache_object_ptr create_cache_object(const table_info& table, object_value value) {
            auto obj_ptr = find_in_cache(value.service);
            bool is_new_ptr = !obj_ptr;

            if (!is_new_ptr) {
                // find object in RAM, and delete it from RAM
                remove_cache_object(*obj_ptr);
            } else if (has_pending_cell()) {
                // find object in pending deletes, and don't add it to RAM
                obj_ptr = find_in_deleted(value.service);
                is_new_ptr = !obj_ptr;
            }

            if (is_new_ptr) {
                // create object, but don't add it to RAM
                obj_ptr = cache_object_ptr(new cache_object(std::move(value)));
            }

            auto service_ptr = find_cache_service(value);
            convert_value(service_ptr, *obj_ptr, std::move(value));
            return obj_ptr;
        }

        void convert_value(cache_service_info* service_ptr, cache_object& cache_obj, const object_value& value) const {
            // set_cache_value happens in three cases:
            //   1. create object for interchain tables ->  value.is_null()
            //   2. loading object from chaindb         -> !value.is_null()
            //   3. restore from undo state             -> !value.is_null()
            if (service_ptr && service_ptr->converter && !value.is_null()) {
                service_ptr->converter->convert_variant(cache_obj, value);
            }
        }

        void add_ram_usage(cache_cell& cell, const int64_t delta) {
            if (!delta || cell.kind != cache_cell::LRU) {
                return;
            }

            cell.size = add_ram_usage(cell.size, delta);
            ram_used_ = add_ram_usage(ram_used_, delta);
        }

        uint64_t add_ram_usage(uint64_t usage, const int64_t delta) {
            CYBERWAY_CACHE_ASSERT(delta <= 0 || UINT64_MAX - usage >= (uint64_t)delta,
                "Cache delta would overflow UINT64_MAX", ("delta", delta)("used", usage));
            CYBERWAY_CACHE_ASSERT(delta >= 0 || usage >= (uint64_t)(-delta),
                "Cache delta would underflow UINT64_MAX", ("delta", delta)("used", usage));

            return usage + delta;
        }
    }; // struct table_cache_map

    //---------------------------------------

    cache_object::cache_object(object_value obj) {
        object_ = std::move(obj);
    }

    bool cache_object::is_valid_cell() const {
        return !state_ || (state_->cell && state_->cell->map);
    }

    bool cache_object::has_cell() const {
        assert(is_valid_cell());
        return !!state_;
    }

    bool cache_object::is_same_cell(const cache_cell& cell) const {
        assert(is_valid_cell());
        return state_ && &cell == state_->cell;
    }

    cache_object_state& cache_object::state() {
        assert(has_cell());
        return *state_;
    }

    cache_cell& cache_object::cell() {
        assert(has_cell());
        return *state_->cell;
    }

    cache_map_impl& cache_object::map() {
        assert(has_cell());
        return *state_->cell->map;
    }

    cache_object_state* cache_object::swap_state(cache_object_state& state) {
        assert(!is_same_cell(*state.cell));

        auto tmp = state_;
        state_ = &state;
        return tmp;
    }

    bool cache_object::is_deleted() const {
        if (!has_cell()) {
            return true;
        }

        auto pending_ptr = pending_cache_object_state::cast(state_);
        if (pending_ptr) {
            return pending_ptr->is_deleted;
        }

        return false;
    }

    //---------------------------------------

    cache_map::cache_map()
    : impl_(std::make_unique<cache_map_impl>()) {
    }

    cache_map::~cache_map() = default;

    void cache_map::set_cache_converter(const table_info& info, const cache_converter_interface& converter) const  {
        impl_->set_cache_converter(cache_service_key(info), converter);
    }

    cache_object_ptr cache_map::create(const table_info& info, const storage_payer_info& storage) const {
        auto pk = impl_->get_next_pk(info);
        if (BOOST_LIKELY(primary_key::Unset != pk)) {
            return create(info, pk, storage);
        }
        return {};
    }

    cache_object_ptr cache_map::create(
        const table_info& info, const primary_key_t pk, const storage_payer_info& storage
    ) const {
        CYBERWAY_ASSERT(primary_key::is_good(pk), cache_primary_key_exception,
            "Value ${pk} can't be used as primary key", ("pk", pk));
        auto obj = object_value{info.to_service(pk), {}};
        obj.service.payer  = storage.owner;
        obj.service.in_ram = true;
        return impl_->emplace(info, std::move(obj));
    }

    void cache_map::destroy(cache_object& obj) const {
        return impl_->remove_cache_object(obj);
    }

    void cache_map::set_next_pk(const table_info& table, const primary_key_t pk) const {
        impl_->set_next_pk(table, pk);
    }

    cache_object_ptr cache_map::find(const service_state& service) const {
        return impl_->find(service);
    }

    cache_object_ptr cache_map::find(
        const service_state& service, const index_name_t index, const char* value, const size_t size
    ) const {
        return impl_->find({service, index, value, size});
    }

    cache_object_ptr cache_map::emplace(const table_info& table, object_value obj) const {
        assert(obj.pk() != primary_key::Unset && !obj.is_null());
        return impl_->emplace(table, std::move(obj));
    }

    void cache_map::remove(const table_info& table, const primary_key_t pk) const {
        assert(pk != primary_key::Unset);
        impl_->remove_cache_object(table.to_service(pk));
    }

    void cache_map::set_object(const table_info& table, cache_object& cache_obj, object_value value) const {
        impl_->set_object(table, cache_obj, std::move(value));
    }

    void cache_map::set_service(const table_info& table, cache_object& cache_obj, service_state service) const {
        impl_->set_service(table, cache_obj, std::move(service));
    }

    void cache_map::set_revision(const object_value& obj, const revision_t rev) const {
        assert(obj.pk() != primary_key::Unset && impossible_revision != rev);
        impl_->set_revision(obj.service, rev);
    }

    uint64_t cache_map::calc_ram_bytes(const revision_t revision) const {
        return impl_->calc_ram_bytes(revision);
    }

    void cache_map::set_revision(const revision_t revision) const {
        impl_->set_revision(revision);
    }

    void cache_map::start_session(const revision_t revision) const {
        impl_->start_session(revision);
    }

    void cache_map::push_session(const revision_t revision) const {
        impl_->push_session(revision);
    }

    void cache_map::squash_session(const revision_t revision) const {
        impl_->squash_session(revision);
    }

    void cache_map::undo_session(const revision_t revision) const {
        impl_->undo_session(revision);
    }

    void cache_map::clear() const {
        impl_->clear();
    }

} } // namespace cyberway::chaindb
