#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/abi_info.hpp>
#include <cyberway/chaindb/exception.hpp>

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

    struct cache_object_compare {
        bool operator()(const service_state& l, const service_state& r) const {
            if (l.pk    < r.pk   ) return true;
            if (l.pk    > r.pk   ) return false;

            if (l.table < r.table) return true;
            if (l.table > r.table) return false;

            if (l.code  < r.code ) return true;
            if (l.code  > r.code ) return false;

            return l.scope < r.scope;
        }

        bool operator()(const cache_object& l, const cache_object& r) const {
            return (*this)(l.object().service, r.object().service);
        }

        bool operator()(const cache_object& l, const service_state& r) const {
            return (*this)(l.object().service, r);
        }

        bool operator()(const service_state& l, const cache_object& r) const {
            return (*this)(l, r.object().service);
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
        : code(tab.code), table(tab.table->name) {
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
        primary_key_t next_pk = unset_primary_key;
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
        const index_info& info;
        const char*       blob;
        const size_t      size;
    }; // struct cache_index_key

    struct cache_index_compare {
        const index_name&   index(const cache_index_value& v) const { return v.index;                   }
        const index_name&   index(const cache_index_key& v  ) const { return v.info.index->name;        }

        const table_name&   table(const cache_index_value& v) const { return v.object->service().table; }
        const table_name&   table(const cache_index_key& v  ) const { return v.info.table->name;        }

        const account_name& code (const cache_index_value& v) const { return v.object->service().code;  }
        const account_name& code (const cache_index_key& v  ) const { return v.info.code;               }

        const account_name& scope(const cache_index_value& v) const { return v.object->service().scope; }
        const account_name& scope(const cache_index_key& v  ) const { return v.info.scope;              }

        const size_t        size (const cache_index_value& v) const { return v.blob.size();             }
        const size_t        size (const cache_index_key& v  ) const { return v.size;                    }

        const char*         data (const cache_index_value& v) const { return v.blob.data();             }
        const char*         data (const cache_index_key& v  ) const { return v.blob;                    }

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
                tmp_obj_ptr->release();
                return;
            }

            tmp_obj_ptr->swap_state(*tmp_prev_state);

            auto pending_prev_state = cast(tmp_prev_state);
            if (pending_prev_state) {
                pending_prev_state->is_deleted = is_deleted;
            }

            prev_state = nullptr;
        }

        void commit() {
            if (!object_ptr) {
                return;
            }

            while (prev_state) {
                prev_state->object_ptr.reset();

                auto pending_prev_state = cast(prev_state);
                if (pending_prev_state) {
                    prev_state = pending_prev_state->prev_state;
                } else {
                    prev_state = nullptr;
                }
            }

            if (is_deleted) {
                object_ptr->release();
                object_ptr.reset();
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

        void emplace(cache_object_ptr obj_ptr, const bool is_deleted) {
            assert(obj_ptr);
            assert(!obj_ptr->has_cell() || obj_ptr->cell().kind == cache_cell::LRU);

            auto& state = emplace_impl(std::move(obj_ptr), is_deleted);
            add_ram_usage(state);
        }

        void emplace(cache_object_ptr obj_ptr, const bool is_deleted, cache_object_state* prev_state) {
            assert(obj_ptr);
            assert(obj_ptr->cell().kind == cache_cell::Pending);

            // squash case - don't add ram usage
            auto& state = emplace_impl(std::move(obj_ptr), is_deleted);
            state.prev_state = prev_state;
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
            auto delta = max_distance_;
            if (state.prev_state) {
                delta = pos - state.prev_state->cell->pos;
            }

            CYBERWAY_CACHE_ASSERT(UINT64_MAX - size >= delta, "Pending delta would overflow UINT64_MAX");
            size += delta;
        }

        pending_cache_object_state& emplace_impl(cache_object_ptr obj_ptr, const bool is_deleted) {
            assert(obj_ptr);

            state_list.emplace_back(*this, std::move(obj_ptr));

            auto& state = state_list.back();
            auto  prev_state = state.object_ptr->swap_state(state);

            state.prev_state = prev_state;
            state.is_deleted = is_deleted;

            return state;
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
                object_ptr->mark_deleted();
            }
            object_ptr.reset();
        }
    }; // struct lru_cache_object_state

    struct lru_cache_cell final: public cache_cell {
        lru_cache_cell(cache_map_impl& m)
        : cache_cell(m, cache_cell::LRU) {
        }

        ~lru_cache_cell() {
            clear();
        }

        void clear() {
            for (auto& state: state_list) if (state.object_ptr) {
                state.reset();
            }
            state_list.clear();
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
                state.object_ptr->mark_deleted();
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
        cache_map_impl(abi_map& map)
        : abi_map_(map),
          system_cell_(*this) {
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

        cache_object_ptr emplace(object_value value) {
            if (!value.service.in_ram) {
                return create_cache_object(std::move(value));
            } else {
                return emplace_in_ram(std::move(value));
            }
        }

        void remove_cache_object(const service_state& key) {
            auto obj_ptr = find(key);
            if (obj_ptr) {
                obj_ptr->mark_deleted();
            }
        }

        void set_revision(const service_state& key, const revision_t rev) {
            auto obj_ptr = find(key);
            if (obj_ptr) {
                obj_ptr->set_revision(rev);
            }
        }

        primary_key_t get_next_pk(const table_info& table) {
            auto service_ptr = find_cache_service(table);
            if (service_ptr && unset_primary_key != service_ptr->next_pk) {
                return service_ptr->next_pk++;
            }
            return unset_primary_key;
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

            cache_index_tree_.clear();
            cache_object_tree_.clear();

            service_tree_type tmp_tree;
            tmp_tree.swap(service_tree_);
            service_tree_.reserve(tmp_tree.size());
            for (auto service: tmp_tree) if (service.second.converter) {
                set_cache_converter(service.first, *service.second.converter);
            }
        }

        uint64_t calc_ram_bytes(const revision_t revision) {
            return get_pending_cell(revision).size;
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
            CYBERWAY_CACHE_ASSERT(pending_cell_list_.size() == 1, "Push session for not-empty pending caches");
            auto& pending = get_pending_cell(revision);

            lru_cell_list_.emplace_back(*this);
            auto& lru = lru_cell_list_.back();
            lru.pos = pending.pos;

            for (auto& state: pending.state_list) {
                state.commit();
                if (!state.is_deleted) {
                    add_lru_object(std::move(state.object_ptr));
                }
            }

            pending_cell_list_.pop_back();
            if (!lru.size) {
                lru_cell_list_.pop_back();
            }

            CYBERWAY_CACHE_ASSERT(deleted_object_tree_.empty(), "Tree with deleted object still has items");

            clear_overused_ram();
        }

        void squash_session(const revision_t revision) {
            CYBERWAY_CACHE_ASSERT(pending_cell_list_.size() >= 2, "Squash session for less then 2 pending caches");

            auto itr = pending_cell_list_.end();
            --itr; auto& src_cell =*itr;
            CYBERWAY_CACHE_ASSERT(revision == src_cell.revision(), "Wrong revision of top pending caches");
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
                    dst_cell.emplace(std::move(src_state.object_ptr), src_state.is_deleted, src_state.prev_state);
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

        void release_cache_object(cache_object& obj, cache_indicies& indicies, const bool is_deleted) {
            if (is_deleted) {
                auto itr = deleted_object_tree_.iterator_to(obj);
                deleted_object_tree_.erase(itr);
            } else {
                auto tree_itr = cache_object_tree_.iterator_to(obj);
                cache_object_tree_.erase(tree_itr);
                delete_cache_indicies(indicies);

                auto service_itr = service_tree_.find(obj.object());
                CYBERWAY_CACHE_ASSERT(service_tree_.end() != service_itr, "No service info on release object");

                service_itr->second.cache_object_cnt--;
                if (service_itr->second.empty()) {
                    service_tree_.erase(service_itr);
                }

                add_ram_usage(obj.cell(), -obj.service().size);
            }
        }

        bool delete_cache_object(cache_object& obj, cache_indicies& indicies) {
            release_cache_object(obj, indicies, false);

            if (!has_pending_cell() || obj.cell().kind == cache_cell::System) {
                return true;
            }

            add_pending_object(cache_object_ptr(&obj), true);
            deleted_object_tree_.insert(obj); // to save position in LRU
            return false;
        }

        void change_cache_object(cache_object& obj, const int delta, cache_indicies& indicies) {
            add_ram_usage(obj.cell(), delta);

            if (is_system_code(obj.service().code)) {
                using state_object = eosio::chain::resource_limits::resource_limits_state_object;
                if (obj.has_data() && obj.service().table == chaindb::tag<state_object>::get_code()) {
                    auto& state = multi_index_item_data<state_object>::get_T(obj);
                    ram_limit_ = get_ram_limit(state.virtual_ram_limit, state.reserved_ram_size);
                }

                // don't rebuild indicies for interchain objects
                if (indicies.capacity()) {
                    return;
                }
            }

            delete_cache_indicies(indicies);
            build_cache_indicies(obj, indicies);
        }

    private:
        using cache_object_tree_type = boost::intrusive::set<cache_object>;
        using cache_index_tree_type  = boost::intrusive::set<cache_index_value>;
        using lru_cell_list_type     = std::deque<lru_cache_cell>;
        using pending_cell_list_type = std::deque<pending_cache_cell>;
        using service_tree_type      = fc::flat_map<cache_service_key, cache_service_info>;

        abi_map&               abi_map_;

        cache_object_tree_type cache_object_tree_;
        cache_object_tree_type deleted_object_tree_;
        cache_index_tree_type  cache_index_tree_;
        service_tree_type      service_tree_;

        pending_cell_list_type pending_cell_list_;
        lru_cell_list_type     lru_cell_list_;
        system_cache_cell      system_cell_;

        uint64_t               ram_limit_ = get_ram_limit();
        uint64_t               ram_used_  = 0;

        static uint64_t get_ram_limit(
            const uint64_t limit   = eosio::chain::config::default_virtual_ram_limit,
            const uint64_t reserve = eosio::chain::config::default_reserved_ram_size
        ) {
            // reserve for system objects (transactions, blocks, abi cache, ...)
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
            assert(obj_ptr && obj_ptr->service().payer.empty());
            system_cell_.emplace(std::move(obj_ptr));
        }

        void add_lru_object(cache_object_ptr obj_ptr) {
            assert(obj_ptr);
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
                pending_ptr->emplace(std::move(obj_ptr), false);
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

            pending_ptr->emplace(std::move(obj_ptr), is_deleted);
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

        void build_cache_indicies(cache_object& obj, cache_indicies& indicies) {
            auto& object  = obj.object();
            if (object.is_null()) return;

            auto& service = obj.service();
            auto  ctr = abi_map_.find(service.code);
            if (abi_map_.end() == ctr) return;

            auto& abi = ctr->second;
            auto  ttr = abi.find_table(service.table);
            if (!ttr) return;

            indicies.reserve(ttr->indexes.size() - 1 /* skip primary */);
            auto info = index_info(service.code, service.scope);
            info.table = ttr;

            for (auto& i: ttr->indexes) if (i.unique && i.name != names::primary_index) {
                info.index = &i;

                auto big_blob = abi.to_bytes(info, object.value); // size of this object is 1 Mb
                if (big_blob.empty()) continue;

                auto blob = bytes(big_blob.begin(), big_blob.end()); // minize memory usage
                indicies.emplace_back(i.name, std::move(blob), obj);
                CYBERWAY_ASSERT(cache_index_tree_.end() == cache_index_tree_.find(indicies.back()),
                    driver_duplicate_exception, "Cache duplicate unique records in the index ${index}",
                    ("index", get_full_index_name(info)));
            }

            for (auto& i: indicies) {
                cache_index_tree_.insert(i);
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

        cache_object_ptr emplace_in_ram(object_value value) {
            auto obj_ptr = find_in_cache(value.service);
            bool is_new_ptr = false;
            bool is_del_ptr = false;

            if (!obj_ptr && has_pending_cell()) {
                obj_ptr = find_in_deleted(value.service);
                is_del_ptr = !!obj_ptr;
            }

            if (!obj_ptr) {
                obj_ptr = cache_object_ptr(new cache_object());
                is_new_ptr = true;
            }

            assert(!is_new_ptr || !is_del_ptr);

            auto& service = get_cache_service(value);
            set_cache_value(&service, *obj_ptr.get(), std::move(value));

            if (is_del_ptr) {
                auto itr = deleted_object_tree_.iterator_to(*obj_ptr.get());
                deleted_object_tree_.erase(itr);
                add_pending_object(obj_ptr, false);
            } else if (is_new_ptr) {
                if (value.service.payer.empty()) {
                    add_system_object(obj_ptr);
                } else if (has_pending_cell()) {
                    add_pending_object(obj_ptr);
                } else {
                    add_lru_object(obj_ptr);
                }
            }

            if (is_new_ptr || is_del_ptr) {
                cache_object_tree_.insert(*obj_ptr.get());
                service.cache_object_cnt++;
            }

            return obj_ptr;
        }

        cache_object_ptr create_cache_object(object_value value) {
            auto obj_ptr = find_in_cache(value.service);
            bool is_new_ptr = !obj_ptr;

            if (!is_new_ptr) {
                // find object in RAM, and delete it from RAM
                obj_ptr->mark_deleted();
            } else if (has_pending_cell()) {
                // find object in pending deletes, and don't add it to RAM
                obj_ptr = find_in_deleted(value.service);
                is_new_ptr = !obj_ptr;
            }

            if (is_new_ptr) {
                // create object, but don't add it to RAM
                obj_ptr = cache_object_ptr(new cache_object());
            }

            auto service_ptr = find_cache_service(value);
            set_cache_value(service_ptr, *obj_ptr.get(), std::move(value));
            return obj_ptr;
        }

        void set_cache_value(cache_service_info* service_ptr, cache_object& obj, object_value value) const {
            // set_cache_value happens in three cases:
            //   1. create object for interchain tables ->  value.is_null()
            //   2. loading object from chaindb         -> !value.is_null()
            //   3. restore from undo state             -> !value.is_null()
            if (service_ptr && service_ptr->converter && !value.is_null()) {
                service_ptr->converter->convert_variant(obj, value);
            }

            obj.set_object(std::move(value));
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

    const cache_cell& cache_object::cell() const {
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

    void cache_object::set_object(object_value obj) {
        assert(object_.service.empty() || is_valid_table(obj.service));

        int delta = obj.service.size - object_.service.size;
        object_ = std::move(obj);
        blob_.clear();

        if (has_cell()) {
            map().change_cache_object(*this, delta, indicies_);
        }
    }

    void cache_object::set_service(service_state service) {
        assert(is_valid_table(service));

        object_.service = std::move(service);
        if (!object_.service.in_ram && has_cell()) {
            mark_deleted();
        }
    }

    void cache_object::mark_deleted() {
        assert(!is_deleted());

        auto is_released = map().delete_cache_object(*this, indicies_);
        if (is_released) {
            auto& tmp_state = *state_;
            state_ = nullptr;
            tmp_state.reset();
        }
    }

    void cache_object::release() {
        auto& pending_state = pending_cache_object_state::cast(*state_);
        pending_state.cell->map->release_cache_object(*this, indicies_, pending_state.is_deleted);
        state_ = nullptr;
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

    cache_map::cache_map(abi_map& map)
    : impl_(new cache_map_impl(map)) {
    }

    cache_map::~cache_map() = default;

    void cache_map::set_cache_converter(const table_info& table, const cache_converter_interface& converter) const  {
        impl_->set_cache_converter(cache_service_key(table), converter);
    }

    cache_object_ptr cache_map::create(const table_info& table, const storage_payer_info& storage) const {
        auto pk = impl_->get_next_pk(table);
        if (BOOST_UNLIKELY(unset_primary_key == pk)) {
            return {};
        }
        auto value   = service_state(table, pk);
        value.payer  = storage.payer;
        value.owner  = storage.owner;
        value.in_ram = true;
        return impl_->emplace({std::move(value), {}});
    }

    void cache_map::set_next_pk(const table_info& table, const primary_key_t pk) const {
        impl_->set_next_pk(table, pk);
    }

    cache_object_ptr cache_map::find(const table_info& table, const primary_key_t pk) const {
        return impl_->find({table, pk});
    }

    cache_object_ptr cache_map::find(const index_info& info, const char* value, const size_t size) const {
        return impl_->find({info, value, size});
    }

    cache_object_ptr cache_map::emplace(const table_info& table, object_value obj) const {
        assert(obj.pk() != unset_primary_key && !obj.is_null());
        return impl_->emplace(std::move(obj));
    }

    void cache_map::remove(const table_info& table, const primary_key_t pk) const {
        assert(pk != unset_primary_key);
        impl_->remove_cache_object({table, pk});
    }

    void cache_map::set_revision(const object_value& obj, const revision_t rev) const {
        assert(obj.pk() != unset_primary_key && impossible_revision != rev);
        impl_->set_revision(obj.service, rev);
    }

    uint64_t cache_map::calc_ram_bytes(const revision_t revision) const {
        return impl_->calc_ram_bytes(revision);
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
