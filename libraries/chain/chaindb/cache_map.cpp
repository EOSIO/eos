#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/table_object.hpp>

namespace cyberway { namespace chaindb {

    struct table_cache_object final: public table_object::object {
        table_cache_object(const table_info& src, const cache_converter_interface& converter)
        : object(src),
          converter_(converter) {
        }

        using map_type = fc::flat_map<primary_key_t, cache_item_ptr>;

        map_type map;
        primary_key_t next_pk = unset_primary_key;

        cache_item_ptr emplace(object_value obj) {
            auto pk = obj.pk();
            auto itr = map.find(pk);

            if (map.end() == itr) {
                auto itm = std::make_shared<cache_item>(std::move((obj)));
                itr = map.emplace(pk, std::move(itm)).first;
            } else {
                itr->second->set_object(std::move(obj));
            }
            if (!itr->second->object().is_null()) {
                itr->second->data = converter_.convert_variant(*itr->second, itr->second->object());
            }
            return itr->second;
        }

    private:
        const cache_converter_interface& converter_;
    }; // struct table_cache_object

    using table_cache_object_index = table_object::index<table_cache_object>;

    struct cache_map::cache_map_impl_ final {
        cache_map_impl_() = default;
        ~cache_map_impl_() = default;

        table_cache_object* find(const table_info& table) {
            auto itr = table_object::find(index_, table);
            if (index_.end() != itr) return &const_cast<table_cache_object&>(*itr);

            return nullptr;
        }

        table_cache_object* find_without_scope(const table_info& table) {
            auto itr = table_object::find_without_scope(index_, table);
            if (index_.end() != itr) return &const_cast<table_cache_object&>(*itr);

            return nullptr;
        }

        void set_converter(const table_info& table, const cache_converter_interface& converter) {
            auto cache = find(table);
            if (cache) return;

            table_object::emplace(index_, table, converter);
        }

        void erase(table_cache_object& cache) {
            index_.erase(index_.iterator_to(cache));
        }

        void clear() {
            for (auto& table: index_) {
                const_cast<table_cache_object&>(table).map.clear();
            }
        }

    private:
        table_cache_object_index index_;
    }; // struct cache_map::cache_map_impl_

    cache_map::cache_map()
    : impl_(new cache_map_impl_()) {
    }

    cache_map::~cache_map() = default;

    void cache_map::set_cache_converter(const table_info& table, const cache_converter_interface& converter) const  {
        impl_->set_converter(table, converter);
    }

    cache_item_ptr cache_map::create(const table_info& table) const {
        cache_item_ptr item;
        auto cache_pk = impl_->find_without_scope(table);
        if (BOOST_LIKELY(cache_pk && cache_pk->next_pk != unset_primary_key)) {
            const auto pk = cache_pk->next_pk++;
            auto cache_value = impl_->find(table);
            if (cache_value) {
                return cache_value->emplace({{table, pk}, {}});
            }
        }
        return cache_item_ptr();
    }

    void cache_map::set_next_pk(const table_info& table, const primary_key_t pk) const {
        auto cache = impl_->find_without_scope(table);
        if (!cache) return;

        cache->next_pk = pk;

        // TODO: is it possible case?
//        auto itr = cache->map.lower_bound(pk);
//        for (auto etr = cache->map.end(); itr != etr; ) {
//            cache->map.erase(itr++);
//        }
    }

    cache_item_ptr cache_map::find(const table_info& table, const primary_key_t pk) const {
        cache_item_ptr item;
        auto cache = impl_->find(table);
        if (!cache) return item;

        auto itr = cache->map.find(pk);
        if (cache->map.end() != itr) {
            if (itr->second->is_deleted()) {
                cache->map.erase(itr);
            } else {
                return itr->second;
            }
        }

        return item;
    }

    cache_item_ptr cache_map::emplace(const table_info& table, object_value obj) const {
        if (obj.pk() == unset_primary_key) return cache_item_ptr();

        auto cache = impl_->find(table);
        if (!cache) return cache_item_ptr();

        return cache->emplace(std::move(obj));
    }

    void cache_map::remove(const table_info& table, const primary_key_t pk) const {
        if (pk == unset_primary_key) return;

        auto cache = impl_->find(table);
        if (!cache) return;

        auto itr = cache->map.find(pk);
        if (itr != cache->map.end()) {
            itr->second->mark_deleted();
            cache->map.erase(itr);
        }

        if (cache->map.empty() && cache->code() != account_name()) {
            impl_->erase(*cache);
        }
    }

    void cache_map::clear() {
        impl_->clear();
    }

    //-----------

    cache_converter_interface::cache_converter_interface() = default;

    cache_converter_interface::~cache_converter_interface() = default;

} } // namespace cyberway::chaindb
