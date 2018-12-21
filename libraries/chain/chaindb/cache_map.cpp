#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/table_object.hpp>

namespace cyberway { namespace chaindb {

    struct table_cache_object final: public table_object::object {
        using table_object::object::object;

        using map_type = std::map<primary_key_t, cache_item_ptr>;

        map_type map;
        primary_key_t next_pk = unset_primary_key;
    }; // struct table_cache_object

    using table_cache_object_index = table_object::index<table_cache_object>;

    struct cache_map::cache_map_impl_ final {
        cache_map_impl_() = default;
        ~cache_map_impl_() = default;

        table_cache_object* find(const table_info& table) {
            auto itr = table_object::find(tables, table);
            if (tables.end() != itr) return &const_cast<table_cache_object&>(*itr);

            return nullptr;
        }

        table_cache_object& get(const table_info& table) {
            auto cache = find(table);
            if (cache) return *cache;

            return table_object::emplace(tables, table);
        }

        table_cache_object_index tables;
    }; // struct cache_map::cache_map_impl_

    cache_map::cache_map()
    : impl_(new cache_map_impl_()) {
    }

    cache_map::~cache_map() = default;

    primary_key_t cache_map::get_next_pk(const table_info& table) const {
        auto cache = impl_->find(table);
        if (cache && cache->next_pk != unset_primary_key) return cache->next_pk++;

        return unset_primary_key;
    }

    void cache_map::set_next_pk(const table_info& table, const primary_key_t pk) const {
        auto& cache = impl_->get(table);
        cache.next_pk = pk;
    }

    cache_item_ptr cache_map::find(const table_info& table, const primary_key_t pk) {
        auto cache = impl_->find(table);
        if (cache) {
            auto itr = cache->map.find(pk);
            if (cache->map.end() != itr) return itr->second;
        }

        // not found case:
        return cache_item_ptr();
    }

    void cache_map::insert(const table_info& table, cache_item_ptr item) {
        if (!item) return;

        auto& cache = impl_->get(table);
        auto itr = cache.map.find(item->pk);

        if (cache.map.end() != itr) {
            itr->second = std::move(item);
        } else {
            const auto primary_key = item->pk;
            cache.map.emplace(primary_key, std::move(item));
        }
    }

    void cache_map::update(const table_info& table, const primary_key_t pk, const variant& value) {
        if (pk == unset_primary_key) return;

        auto cache = impl_->find(table);
        if (!cache) return;

        auto itr = cache->map.find(pk);
        if (cache->map.end() == itr) return;

        itr->second->convert_variant(value);
    }

    void cache_map::remove(const table_info& table, const primary_key_t pk) {
        if (pk == unset_primary_key) return;

        auto cache = impl_->find(table);
        if (!cache) return;

        cache->map.erase(pk);
    }

    //-----------

    cache_converter_interface::cache_converter_interface() = default;

    cache_converter_interface::~cache_converter_interface() = default;

} } // namespace cyberway::chaindb
