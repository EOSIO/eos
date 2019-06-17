#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/abi_info.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/table_info.hpp>
#include <cyberway/chaindb/typed_name.hpp>

#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/composite_key.hpp>

/** Cache exception is a critical errors and it doesn't handle by chain */
#define CYBERWAY_CACHE_ASSERT(expr, FORMAT, ...)                      \
    FC_MULTILINE_MACRO_BEGIN                                          \
        if (BOOST_UNLIKELY( !(expr) )) {                              \
            elog(FORMAT, __VA_ARGS__);                                \
            FC_THROW_EXCEPTION(cache_exception, FORMAT, __VA_ARGS__); \
        }                                                             \
    FC_MULTILINE_MACRO_END

namespace cyberway { namespace chaindb {
    namespace bmi = boost::multi_index;

    struct cache_object_key final {
        const primary_key_t pk    = primary_key::Unset;
        const scope_name_t  scope = 0;

        cache_object_key(const service_state& v)
        : pk(v.pk), scope(v.scope) {
        }

        cache_object_key(const cache_object& v)
        : pk(v.pk()), scope(v.service().scope) {
        }

        friend bool operator < (const cache_object_key& l, const cache_object_key& r) {
            if (l.pk < r.pk) return true;
            if (l.pk > r.pk) return false;

            return l.scope < r.scope;
        }
    }; // struct cache_object_key

    //----------------------------------------------------------------------------------------------

    struct cache_index_key final {
        const index_name_t index = 0;
        const scope_name_t scope = 0;
        const char*        data  = nullptr;
        const size_t       size  = 0;
    }; // struct cache_index_key

    struct cache_index_value final {
        struct by_key;
        struct by_object;

        const index_name_t  index = 0;
        const bytes         blob;
        cache_object_ptr    object_ptr;

        cache_index_value(const index_name i, bytes b, cache_object_ptr o)
        : index(i), blob(std::move(b)), object_ptr(std::move(o)) {
        }

        cache_index_value(cache_index_value&&) = default;

        ~cache_index_value() = default;

        cache_index_key value_key() const {
            return {index, object_ptr->service().scope, blob.data(), blob.size()};
        }

        static const void* object_key(const cache_object& cache_obj) {
            return static_cast<const void*>(&cache_obj);
        }

        const void* object_key() const {
            return object_key(*object_ptr);
        }

        uint64_t size() const {
            return blob.size() + 64;
        }
    }; // struct cache_index_value


    struct cache_index_compare final {
        const cache_index_key& key(const cache_index_key&   k) const { return k;             }
        cache_index_key        key(const cache_index_value& v) const { return v.value_key(); }

        template<typename LeftKey, typename RightKey> bool operator()(const LeftKey& l, const RightKey& r) const {
            return operator()(key(l), key(r));
        }

        bool operator()(const cache_index_key& l, const cache_index_key& r) const {
            if (l.index < r.index) return true;
            if (l.index > r.index) return false;

            if (l.scope < r.scope) return true;
            if (l.scope > r.scope) return false;

            if (l.size  < r.size ) return true;
            if (l.size  > r.size ) return false;

            return (std::memcmp(l.data, r.data, l.size) < 0);
        }
    }; // struct cache_index_compare

    using cache_index_tree = bmi::multi_index_container<
        cache_index_value,
        bmi::indexed_by<
            bmi::ordered_unique<
                bmi::tag<cache_index_value::by_key>,
                bmi::const_mem_fun<cache_index_value, cache_index_key, &cache_index_value::value_key>,
                cache_index_compare>,
            bmi::ordered_non_unique<
                bmi::tag<cache_index_value::by_object>,
                bmi::const_mem_fun<cache_index_value, const void*, &cache_index_value::object_key>,
                std::less<const void*>>>>;

    //-----------------------------------------------------------------------------------------------

    struct cache_pk_value final {
        struct by_key;
        struct by_object;

        const primary_key_t pk = primary_key::Unset;
        const cache_object_ptr object_ptr;

        cache_pk_value(primary_key_t p, cache_object_ptr o)
        : pk(p), object_ptr(std::move(o)) {
        }

        scope_name_t scope_key() const {
            return object_ptr->service().scope;
        }

        static const void* object_key(const cache_object& cache_obj) {
            return static_cast<const void*>(&cache_obj);
        }

        const void* object_key() const {
            return object_key(*object_ptr);
        }

        uint64_t size() const {
            return 64;
        }
    }; // struct cache_pk_value

    using cache_pk_tree = bmi::multi_index_container<
        cache_pk_value,
        bmi::indexed_by<
            bmi::ordered_unique<
                bmi::tag<cache_pk_value::by_key>,
                bmi::composite_key<cache_pk_value,
                    bmi::member<cache_pk_value, const primary_key_t, &cache_pk_value::pk>,
                    bmi::const_mem_fun<cache_pk_value, scope_name_t, &cache_pk_value::scope_key>>,
                composite_key_compare<
                    std::less<primary_key_t>,
                    std::less<scope_name_t>>>,
            bmi::ordered_non_unique<
                bmi::tag<cache_pk_value::by_object>,
                bmi::const_mem_fun<cache_pk_value, const void*, &cache_pk_value::object_key>,
                std::less<const void*>>>>;

    //-----------------------------------------------------------------------------------------------

    struct cache_service_key final {
        const account_name_t code;
        const table_name_t   table;

        cache_service_key(const service_state& v)
        : code(v.code), table(v.table) {
        }

        cache_service_key(const object_value& v)
        : code(v.service.code), table(v.service.table) {
        }

        cache_service_key(const cache_object& v)
        : code(v.service().code), table(v.service().table) {
        }

        cache_service_key(const table_info& v)
        : code(v.code), table(v.table_name()) {
        }

        friend bool operator < (const cache_service_key& l, const cache_service_key& r) {
            if (l.table < r.table) return true;
            if (l.table > r.table) return false;

            return l.code < r.code;
        }
    }; // cache_service_key

    //-----------------------------------------------------------------------------------------------

    struct cache_service_info final {
        primary_key_t next_pk = primary_key::Unset;
        const cache_converter_interface* converter = nullptr; // exist only for interchain tables

        cache_object_tree object_tree;
        cache_index_tree  index_tree;

        cache_object_tree deleted_object_tree;

        cache_pk_tree     unsuccess_pk_tree;
        cache_index_tree  unsuccess_index_tree;

        cache_service_info() = default;

        cache_service_info(const cache_converter_interface& c)
        : converter(&c) {
        }

        bool empty() const {
            assert(index_tree.empty() || !object_tree.empty());
            return (!converter /* info for contacts tables */ && object_tree.empty() && deleted_object_tree.empty());
        }
    }; // struct cache_service_info

    using cache_service_tree = std::map<cache_service_key, cache_service_info>;

    //-----------------------------------------------------------------------------------------------

    struct lru_cache_object_state final: public cache_object_state {
        using cache_object_state::cache_object_state;

        ~lru_cache_object_state() {
            reset();
        }

        static lru_cache_object_state* cast(cache_object_state* state) {
            if (!state || !state->cell) {
                return nullptr;
            }
            if (cache_cell::Pending == state->kind() || cache_cell::LRU == state->kind()) {
                return static_cast<lru_cache_object_state*>(state);
            }
            return nullptr;
        }

        static lru_cache_object_state& cast(cache_object_state& state) {
            auto lru_ptr = cast(&state);
            assert(lru_ptr);
            return *lru_ptr;
        }

        void commit();
        void reset();

        lru_cache_object_state* prev_state = nullptr;
    }; // struct lru_cache_object_state

    struct lru_cache_cell final: public cache_cell {
        lru_cache_cell(cache_map_impl& m, const uint64_t p, const revision_t r, const uint64_t d)
        : cache_cell(m, p, cache_cell::Pending),
          revision_(r),
          max_distance_(d) {
            assert(revision_ >= start_revision);
            assert(max_distance_ > 0);
        }

        void emplace(cache_object_ptr obj_ptr) {
            assert(obj_ptr);
            assert(cache_cell::Pending == kind());

            auto  lru_prev_state = emplace_impl(std::move(obj_ptr));
            auto& state = state_list.back();

            state.prev_state = lru_prev_state;
            add_ram_bytes(state);
        }

        void squash_revision(lru_cache_cell& src_cell) {
            for (auto& src_state: src_cell.state_list) {
                if (!src_state.prev_state && src_state.object_ptr->is_deleted()) {
                    // undo it
                } else if (!src_state.prev_state || src_state.prev_state->cell != this) {
                    auto  lru_prev_state = emplace_impl(std::move(src_state.object_ptr));
                    auto& state = state_list.back();

                    state.prev_state = lru_prev_state->prev_state;
                }
            }

            add_ram_bytes(src_cell.ram_bytes_);
        }

        int64_t commit_revision() {
            int64_t commited_size = 0;

            kind_ = cache_cell::LRU;

            for (auto& state: state_list) {
                state.commit();
                if (state.object_ptr->is_deleted()) {
                    continue;
                }

                auto delta = state.object_ptr->service().size;
                CYBERWAY_CACHE_ASSERT(UINT64_MAX - commited_size >= delta, "Commiting delta would overflow UINT64_MAX");
                commited_size += delta;
            }

            return commited_size;
        }

        revision_t revision() const {
            return revision_;
        }

        uint64_t ram_bytes() const {
            return ram_bytes_;
        }

        void add_ram_bytes(const uint64_t delta) {
            CYBERWAY_CACHE_ASSERT(UINT64_MAX - ram_bytes_ >= delta, "Pending delta would overflow UINT64_MAX");
            ram_bytes_ += delta;
        }

        std::deque<lru_cache_object_state> state_list; // Expansion of a std::deque is cheaper than the expansion of a std::vector

    private:
        lru_cache_object_state* emplace_impl(cache_object_ptr obj_ptr) {
            state_list.emplace_back(*this, std::move(obj_ptr));

            auto& state = state_list.back();
            auto  prev_state = state.object_ptr->swap_state(state);
            auto  lru_prev_state = lru_cache_object_state::cast(prev_state);
            assert(!prev_state || lru_prev_state);

            return lru_prev_state;
        }

        void add_ram_bytes(const lru_cache_object_state& state) {
            assert(cache_cell::Pending == kind());

            auto object_size = state.object_ptr->service().size;
            if (!object_size) {
                return;
            }

            auto delta = state.prev_state ?
               std::max(
                   eosio::chain::int_arithmetic::safe_prop<uint64_t>(
                       object_size, pos() - state.prev_state->cell->pos(), max_distance_),
                   uint64_t(1)) :
               object_size * config::ram_load_multiplier;

            add_ram_bytes(delta);
        }

        uint64_t ram_bytes_ = 0;
        revision_t revision_ = impossible_revision;
        const uint64_t max_distance_ = 0;
    }; // struct lru_cache_cell

    //-----------------------------------------------------------------------------------------------

    struct system_cache_object_state final: public cache_object_state {
        using cache_object_state::cache_object_state;

        ~system_cache_object_state() {
            reset();
        }

        static system_cache_object_state* cast(cache_object_state* state) {
            if (state && state->cell && cache_cell::System == state->kind()) {
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
        : cache_cell(m, 0, cache_cell::System) {
        }

        ~system_cache_cell() {
            clear();
        }

        void clear();

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

    //-----------------------------------------------------------------------------------------------

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
            if (!primary_key::is_good(key.pk)) {
                return {};
            }

            auto obj_ptr = find_in_cache(find_cache_service(key), key);
            if (obj_ptr) {
                add_pending_object(obj_ptr);
            }
            return obj_ptr;
        }

        cache_object_ptr find(
            const service_state& service, const index_name_t index, const char* value, const size_t size
        ) {
            auto service_ptr = find_cache_service(service);

            if (!service_ptr) {
                return {};
            }

            cache_index_key key{index, service.scope, value, size};
            auto& idx = service_ptr->index_tree.get<cache_index_value::by_key>();
            auto  itr = idx.find(key);
            if (idx.end() != itr && primary_key::is_good(itr->object_ptr->pk())) {
                add_pending_object(itr->object_ptr);
                return itr->object_ptr;
            }

            return {};
        }

        cache_object_ptr find_unsuccess(const service_state& service) {
            auto service_ptr = find_cache_service(service);
            if (!service_ptr) {
                return {};
            }

            auto& idx = service_ptr->unsuccess_pk_tree.get<cache_pk_value::by_key>();
            auto  itr = idx.find(std::make_tuple(service.pk, service.scope));

            if (idx.end() == itr) {
                return {};
            }

            return itr->object_ptr;
        }

        cache_object_ptr find_unsuccess(
            const service_state& service, const index_name_t index, const char* value, const size_t size
        ) {
            auto service_ptr = find_cache_service(service);
            if (!service_ptr) {
                return {};
            }

            cache_index_key key{index, service.scope, value, size};
            auto& idx = service_ptr->unsuccess_index_tree.get<cache_index_value::by_key>();
            auto  itr = idx.find(key);

            if (idx.end() == itr) {
                return {};
            }

            return itr->object_ptr;
        }

        cache_object_ptr emplace(const table_info& table, object_value value) {
            if (!value.service.in_ram) {
                return create_cache_object(table, std::move(value));
            } else {
                return emplace_in_ram(table, std::move(value));
            }
        }

        void emplace_unsuccess(const table_info& table, const primary_key_t pk, const primary_key_t rpk) {
            emplace_unsuccess(
                table.to_service(rpk),
                [&](cache_service_info& service) -> auto& {
                    return service.unsuccess_pk_tree.get<cache_pk_value::by_key>();
                },
                [&](auto& idx) -> auto {
                    return idx.find(std::make_tuple(pk, table.scope));
                },
                [&](auto& idx, cache_object_ptr obj_ptr) {
                    idx.insert(cache_pk_value(pk, std::move(obj_ptr)));
                }
            );
        }

        void emplace_unsuccess(const index_info& index, const char* value, const size_t size, const primary_key_t rpk) {
            emplace_unsuccess(
                index.to_service(rpk),
                [&](cache_service_info& service) -> auto& {
                    return service.unsuccess_index_tree.get<cache_index_value::by_key>();
                },
                [&](auto& idx) -> auto {
                    cache_index_key key{index.index_name(), index.scope, value, size};
                    return idx.find(key);
                },
                [&](auto& idx, cache_object_ptr obj_ptr) {
                    bytes b(value, value + size);
                    cache_index_value value(index.index_name(), std::move(b), std::move(obj_ptr));
                    idx.insert(std::move(value));
                }
            );
        }

        void clear_unsuccess(const service_state& key) {
            auto service_ptr = find_cache_service(key);
            if (service_ptr) {
                service_ptr->unsuccess_pk_tree.clear();
                service_ptr->unsuccess_index_tree.clear();
            }
        }

        void remove_cache_object(const service_state& key) {
            auto service_ptr = find_cache_service(key);
            if (!service_ptr) {
                return;
            }

            auto obj_ptr = find_in_cache(*service_ptr, key);
            if (!obj_ptr) {
                return;
            }

            remove_cache_object(*service_ptr, *obj_ptr);
        }

        void remove_cache_object(cache_object& cache_obj) {
            auto service_ptr = find_cache_service(cache_service_key(cache_obj));
            CYBERWAY_CACHE_ASSERT(service_ptr, "No service info on remove object");

            remove_cache_object(*service_ptr, cache_obj);
        }

        void release_cache_object(cache_object& cache_obj) {
            auto service_ptr = find_cache_service(cache_service_key(cache_obj));
            CYBERWAY_CACHE_ASSERT(service_ptr, "No service info on release object");

            release_cache_object(*service_ptr, cache_obj, true);
        }

        void set_object(const table_info& table, cache_object& cache_obj, object_value value) {
            set_object(table, find_cache_service(table), cache_obj, std::move(value));
        }

        void set_service(const table_info&, cache_object& cache_obj, service_state service) {
            assert(cache_obj.is_valid_table(service));

            cache_obj.object_.service = std::move(service);
            if (!cache_obj.service().in_ram && !cache_obj.is_deleted()) {
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
            return get_pending_cell(revision).ram_bytes();
        }

        void set_revision(const revision_t revision) {
            lru_revision_ = revision;
        }

        void start_session(const revision_t revision) {
            uint64_t pos = 0;

            if (!lru_cell_list_.empty()) {
                auto& lru_cell = lru_cell_list_.back();
                pos = lru_cell.pos();
                if (lru_cell.kind() == cache_cell::LRU) {
                    pos += lru_cell.size;
                }
            }

            lru_cell_list_.emplace_back(*this, pos, revision, ram_limit_);
            pending_cell_list_.emplace_back(&lru_cell_list_.back());
        }

        void push_session(const revision_t revision) {
            CYBERWAY_CACHE_ASSERT(has_pending_cell(), "Push session for empty pending caches");
            auto& pending = get_pending_cell(revision);

            set_revision(revision - 1);
            auto size = pending.commit_revision();

            add_ram_usage(pending, size);

            pending_cell_list_.pop_back();
            CYBERWAY_CACHE_ASSERT(pending_cell_list_.empty(), "After pushing session still exist pending caches");

            clear_overused_ram();
        }

        void squash_session(const revision_t revision) {
            CYBERWAY_CACHE_ASSERT(has_pending_cell(), "Squash session for empty pending caches");

            auto itr = pending_cell_list_.end();
            --itr; auto& src_cell = *(*itr);
            CYBERWAY_CACHE_ASSERT(revision == src_cell.revision(), "Wrong revision of top pending caches");

            // replay and squashing system transaction to LRU
            if (pending_cell_list_.begin() == itr) {
                CYBERWAY_CACHE_ASSERT(revision - 1 == lru_revision_,
                    "Wrong revision on squashing of sys-transaction to replayed block");
                return push_session(revision);
            }

            --itr; auto& dst_cell = *(*itr);
            CYBERWAY_CACHE_ASSERT(revision - 1 == dst_cell.revision(), "Wrong revision of pending caches");

            dst_cell.squash_revision(src_cell);
            pending_cell_list_.pop_back();
        }

        void undo_session(const revision_t revision) {
            if (!has_pending_cell()) {
                // case of pop_block
                return;
            }

            auto& pending = get_pending_cell(revision);
            pending_cell_list_.pop_back();
            lru_cell_list_.pop_back();
        }

    private:
        using lru_cell_list_type     = std::deque<lru_cache_cell>;
        using pending_cell_list_type = std::deque<lru_cache_cell*>;

        cache_service_tree     service_tree_;

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

        bool add_system_object(cache_object_ptr obj_ptr) {
            assert(obj_ptr && is_system_object(*obj_ptr));
            system_cell_.emplace(std::move(obj_ptr));
            return true;
        }

        bool add_pending_object(cache_object_ptr obj_ptr) {
            assert(obj_ptr);

            auto pending_ptr = find_pending_cell();
            if (!pending_ptr) {
                return false;
            }

            if (!obj_ptr->has_cell() || cache_cell::LRU == obj_ptr->kind()) {
                pending_ptr->emplace(std::move(obj_ptr));
                return true;
            }

            return false;
        }

        lru_cache_cell* find_pending_cell() {
            if (!has_pending_cell()) {
                return nullptr;
            }
            return pending_cell_list_.back();
        }

        bool has_pending_cell() const {
            return !pending_cell_list_.empty();
        }

        lru_cache_cell& get_pending_cell(const revision_t revision) {
            auto pending_ptr = find_pending_cell();
            CYBERWAY_CACHE_ASSERT(pending_ptr && pending_ptr->revision() == revision,
                "Wrong pending revision ${pending_revision} != ${revision}",
                ("pending_revision", pending_ptr ? pending_ptr->revision() : impossible_revision)("revision", revision));
            return *pending_ptr;
        }

        cache_service_info* find_cache_service(const cache_service_key& key) {
            auto itr = service_tree_.find(key);
            if (service_tree_.end() != itr) {
                return &itr->second;
            }
            return nullptr;
        }

        void build_cache_indicies(const table_info& table, cache_service_info& service, cache_object& cache_obj) {
            if (cache_obj.object().is_null()) {
                return;
            }

            std::vector<cache_index_value> indicies;
            indicies.reserve(table.table->indexes.size() + 10);

            auto  index = index_info(cache_obj.service().code, cache_obj.service().scope);
            index.table = table.table;

            auto& key_idx = service.index_tree.get<cache_index_value::by_key>();
            for (auto& idx: table.table->indexes) if (idx.unique && idx.name != names::primary_index) {
                index.index = &idx;

                auto blob = table.abi().to_bytes(index, cache_obj.object().value);
                if (blob.empty()) {
                    continue;
                }

                indicies.emplace_back(idx.name, std::move(blob), cache_object_ptr(&cache_obj));
                CYBERWAY_ASSERT(key_idx.end() == key_idx.find(indicies.back()),
                    driver_duplicate_exception, "Cache duplicate unique records in the index ${index}",
                    ("index", get_full_index_name(index)));
            }

            for (auto& value: indicies) {
                service.index_tree.insert(std::move(value));
            }
        }

        void delete_cache_indicies(cache_service_info& service, cache_object& cache_obj) {
            auto& idx = service.index_tree.get<cache_index_value::by_object>();
            auto  key = cache_index_value::object_key(cache_obj);
            auto  itr = idx.lower_bound(key);

            for (; idx.end() != itr && itr->object_key() == key; ) {
                itr = idx.erase(itr);
            }
        }

        void delete_unsuccess_pk(cache_service_info& service, cache_object& cache_obj) {
            auto& idx = service.unsuccess_pk_tree.get<cache_pk_value::by_object>();
            auto  key = cache_pk_value::object_key(cache_obj);
            auto  itr = idx.lower_bound(key);

            for (; idx.end() != itr && itr->object_key() == key; ) {
                itr = idx.erase(itr);
            }
        }

        void delete_unsuccess_index(cache_service_info& service, cache_object& cache_obj) {
            auto& idx = service.unsuccess_index_tree.get<cache_index_value::by_object>();
            auto  key = cache_index_value::object_key(cache_obj);
            auto  itr = idx.lower_bound(key);

            for (; idx.end() != itr && itr->object_key() == key; ) {
                itr = idx.erase(itr);
            }
        }

        void set_object(
            const table_info& table, cache_service_info* service_ptr, cache_object& cache_obj, object_value value
        ) {
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
            }

            if (service_ptr) {
                delete_cache_indicies(*service_ptr, cache_obj);
                build_cache_indicies(table, *service_ptr, cache_obj);
            }
        }

        void release_cache_object(cache_service_info& service, cache_object& cache_obj, bool can_remove_service) {
            auto stage = cache_obj.stage();
            cache_obj.stage_ = cache_object::Released;

            switch (stage) {
                case cache_object::Deleted: {
                    service.deleted_object_tree.erase(cache_object_key(cache_obj));
                    break;
                }

                case cache_object::Active: {
                    service.object_tree.erase(cache_object_key(cache_obj));
                    delete_cache_indicies(service, cache_obj);
                    delete_unsuccess_pk(service, cache_obj);
                    delete_unsuccess_index(service, cache_obj);

                    if (cache_obj.has_cell()) {
                        add_ram_usage(cache_obj.cell(), -cache_obj.service().size);
                    }
                    break;
                }

                case cache_object::Released:
                    return;

                default:
                    assert(false);
            }

            if (can_remove_service && service.empty()) {
                service_tree_.erase(cache_service_key(cache_obj));
            }
        }

        void remove_cache_object(cache_service_info& service, cache_object& cache_obj) {
            assert(!cache_obj.is_deleted());

            bool can_remove_service = (!has_pending_cell() || cache_obj.kind() == cache_cell::System);

            release_cache_object(service, cache_obj, can_remove_service);

            if (can_remove_service) {
                auto& tmp_state = *cache_obj.state_;
                cache_obj.state_ = nullptr;
                tmp_state.reset();
                return;
            }

            // to save position in LRU
            cache_obj.stage_ = cache_object::Deleted;

            auto cache_obj_ptr = cache_object_ptr(&cache_obj);
            add_pending_object(cache_obj_ptr);
            service.deleted_object_tree.emplace(cache_object_key(cache_obj), std::move(cache_obj_ptr));
        }

        cache_object_ptr find_in_cache(const cache_service_info& service, const service_state& key) {
            auto itr = service.object_tree.find(key);
            if (service.object_tree.end() != itr) {
                return itr->second;
            }
            return {};
        }

        cache_object_ptr find_in_cache(const cache_service_info* service_ptr, const service_state& key) {
            if (service_ptr) {
                return find_in_cache(*service_ptr, key);
            }
            return {};
        }

        bool is_system_object(const cache_object& cache_obj) const {
            return !cache_obj.service().payer;
        }

        template<typename Get, typename Find, typename Emplace>
        void emplace_unsuccess(const service_state& key, Get&& get, Find&& find, Emplace&& emplace) {
            if (!has_pending_cell()) {
                return;
            }

            auto service_ptr = find_cache_service(key);
            if (!service_ptr) {
                return;
            }

            auto cache_obj_ptr = find_in_cache(*service_ptr, key);
            if (primary_key::End == key.pk) {
                object_value obj;
                obj.service = key;

                cache_obj_ptr = new cache_object(std::move(obj));
                cache_obj_ptr->stage_ = cache_object::Active;
                service_ptr->object_tree.emplace(cache_object_key(*cache_obj_ptr), cache_obj_ptr);
            }

            if (!cache_obj_ptr) {
                return;
            }

            bool is_pended = false;
            if (!is_system_object(*cache_obj_ptr)) {
                is_pended = add_pending_object(cache_obj_ptr);
            }

            auto& idx = get(*service_ptr);
            auto  itr = find(idx);

            if (idx.end() != itr) {
                if (key.pk == itr->object_ptr->pk()) {
                    if (is_pended) {
                        find_pending_cell()->add_ram_bytes(itr->size());
                    }
                    return;
                }

                idx.erase(itr);
            }

            emplace(idx, std::move(cache_obj_ptr));

            if (is_pended ) {
                find_pending_cell()->add_ram_bytes(itr->size());
            }
        }

        cache_object_ptr emplace_in_ram(const table_info& table, object_value value) {
            auto& service       = service_tree_.emplace(cache_service_key(value), cache_service_info()).first->second;
            auto  cache_obj_ptr = find_in_cache(service, value.service);
            bool  is_new_ptr    = false;
            bool  is_del_ptr    = false;

            if (!cache_obj_ptr && has_pending_cell()) {
                auto itr = service.deleted_object_tree.find(value.service);
                if (service.deleted_object_tree.end() != itr) {
                    cache_obj_ptr = std::move(itr->second);
                    is_del_ptr    = true;
                    service.deleted_object_tree.erase(itr);
                }
            }

            if (!cache_obj_ptr) {
                object_value obj;
                obj.service = value.service;
                obj.service.size = 0;

                cache_obj_ptr = new cache_object(std::move(obj));
                is_new_ptr    = true;
            }

            assert(!is_new_ptr || !is_del_ptr);

            auto& cache_obj = *cache_obj_ptr;
            convert_value(&service, cache_obj, value);

            if (is_system_object(cache_obj)) {
                if (is_new_ptr) {
                    add_system_object(cache_obj_ptr);
                }
            } else {
                add_pending_object(cache_obj_ptr);
            }

            if ((is_new_ptr || is_del_ptr) && cache_obj.has_cell()) {
                cache_obj.stage_ = cache_object::Active;
                service.object_tree.emplace(cache_object_key(cache_obj), cache_obj_ptr);
            }

            set_object(table, &service, cache_obj, std::move(value));

            return cache_obj_ptr;
        }

        cache_object_ptr create_cache_object(const table_info& table, object_value value) {
            auto service_ptr = find_cache_service(value);
            auto obj_ptr     = find_in_cache(service_ptr, value.service);
            bool is_new_ptr  = !obj_ptr;

            if (!is_new_ptr) {
                // find object in RAM, and delete it from RAM
                remove_cache_object(*obj_ptr);
            } else if (has_pending_cell() && service_ptr) {
                // find object in pending deletes, and don't add it to RAM
                auto itr = service_ptr->deleted_object_tree.find(value.service);

                if (service_ptr->deleted_object_tree.end() != itr) {
                    obj_ptr = itr->second;
                    is_new_ptr = true;
                }
            }

            if (is_new_ptr) {
                // create object, but don't add it to RAM
                obj_ptr = new cache_object(value);
            }

            convert_value(service_ptr, *obj_ptr, value);
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
            if (!delta || cell.kind() != cache_cell::LRU) {
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

    //-----------------------------------------------------------------------------------------------

    void lru_cache_object_state::commit() {
        if (!object_ptr) {
            return;
        }

        if (prev_state) {
            prev_state->object_ptr.reset();
        }
        prev_state = nullptr;

        if (object_ptr->is_deleted()) {
            object_ptr->state_ = nullptr;
            cell->map->release_cache_object(*object_ptr);
        }
    }

    void lru_cache_object_state::reset() {
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
            tmp_obj_ptr->state_ = nullptr;
            cell->map->release_cache_object(*tmp_obj_ptr);
            return;
        }

        tmp_obj_ptr->swap_state(*tmp_prev_state);
    }

    //-----------------------------------------------------------------------------------------------

    void system_cache_object_state::reset() {
        if (!object_ptr) {
            return;
        }

        assert(!object_ptr->has_cell());

        auto system_cell = static_cast<system_cache_cell*>(cell);
        auto system_itr  = itr;
        itr = system_cell->state_list.end();

        system_cell->size -= object_ptr->service().size;

        object_ptr.reset();

        if (system_cell->state_list.end() != system_itr) {
            system_cell->state_list.erase(system_itr);
        }
    }

    void system_cache_cell::clear() {
        for (auto& state: state_list) if (state.object_ptr) {
            assert(state.object_ptr->is_same_cell(*this));
            state.itr = state_list.end();
            state.object_ptr->state_ = nullptr;
            map->release_cache_object(*state.object_ptr);
        }

        state_list.clear();
    }

    //-----------------------------------------------------------------------------------------------

    void cache_object_state::reset() {
        switch (kind()) {
            case cache_cell::System:
                system_cache_object_state::cast(*this).reset();
                break;

            case cache_cell::Pending:
            case cache_cell::LRU: {
                lru_cache_object_state::cast(*this).reset();
                break;
            }

            case cache_cell::Unknown:
            default:
                assert(false);
        }
    }

    cache_cell::cache_cell_kind cache_object_state::kind() const {
        assert(cell);
        return cell->kind();
    }

    //-----------------------------------------------------------------------------------------------

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

    cache_object_state& cache_object::state() const {
        assert(has_cell());
        return *state_;
    }

    cache_cell& cache_object::cell() const {
        assert(has_cell());
        return *state_->cell;
    }

    cache_cell::cache_cell_kind cache_object::kind() const {
        return cell().kind();
    }

    cache_map_impl& cache_object::map() const {
        assert(has_cell());
        return *state_->cell->map;
    }

    cache_object_state* cache_object::swap_state(cache_object_state& state) {
        assert(!is_same_cell(*state.cell));

        auto tmp = state_;
        state_ = &state;
        return tmp;
    }

    //-----------------------------------------------------------------------------------------------

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

        CYBERWAY_ASSERT(!impl_->find(obj.service), driver_duplicate_exception,
            "Duplicate unique records in the cache table ${table}", ("table", get_full_table_name(info)));

        return impl_->emplace(info, std::move(obj));
    }

    void cache_map::destroy(cache_object& obj) const {
        impl_->remove_cache_object(obj);
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
        assert(value);
        assert(size);

        return impl_->find(service, index, value, size);
    }

    cache_object_ptr cache_map::emplace(const table_info& table, object_value obj) const {
        assert(obj.pk() != primary_key::Unset && !obj.is_null());
        return impl_->emplace(table, std::move(obj));
    }

    void cache_map::remove(const table_info& table, const primary_key_t pk) const {
        assert(pk != primary_key::Unset);
        impl_->remove_cache_object(table.to_service(pk));
    }

    void cache_map::emplace_unsuccess(const table_info& table, const primary_key_t pk, const primary_key_t rpk) const {
        assert(pk != rpk);
        impl_->emplace_unsuccess(table, pk, rpk);
    }

    void cache_map::emplace_unsuccess(
        const index_info& index, const char* value, const size_t size, const primary_key_t rpk
    ) const {
        assert(value);
        assert(size);
        impl_->emplace_unsuccess(index, value, size, rpk);
    }

    void cache_map::clear_unsuccess(const table_info& table) const {
        impl_->clear_unsuccess(table.to_service());
    }

    cache_object_ptr cache_map::find_unsuccess(const service_state& key) const {
        return impl_->find_unsuccess(key);
    }

    cache_object_ptr cache_map::find_unsuccess(
        const service_state& key, const index_name_t index, const char* value, const size_t size
    ) const {
        assert(value);
        assert(size);
        return impl_->find_unsuccess(key, index, value, size);
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
