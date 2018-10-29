#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/indexed_by.hpp>

#include <cyberway/chaindb/cache_map.hpp>
#include <cyberway/chaindb/controller.hpp>

#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb {

    class cache_table_object final {
        const table_def table_def_;  // <- copy to replace pointer in info_
        const order_def pk_order_;   // <- copy to replace pointer in info_
        table_info      info_;       // <- for const reference

    public:
        const account_name code;
        const account_name scope;
        const table_name   table;
        const field_name   pk_field; // <- for case when contract change its primary key

        cache_table_object(const table_info& src)
        : table_def_(*src.table),
          pk_order_(*src.pk_order),
          info_(src),
          code(src.code),
          scope(src.scope),
          table(src.table->name),
          pk_field(src.pk_order->field) {
            info_.table = &table_def_;
            info_.pk_order = &pk_order_;
        }

        const table_info& info() const {
            return info_;
        }

        using map_type = std::map<primary_key_t, cache_item_ptr>;

        map_type map;
        primary_key_t next_pk = unset_primary_key;
    }; // struct cache_table

    namespace bmi = boost::multi_index;

    using cache_table_index = bmi::multi_index_container<
        cache_table_object,
        bmi::indexed_by<
            bmi::ordered_unique<
                bmi::composite_key<
                    cache_table_object,
                    bmi::member<cache_table_object, const account_name, &cache_table_object::code>,
                    bmi::member<cache_table_object, const account_name, &cache_table_object::scope>,
                    bmi::member<cache_table_object, const table_name,   &cache_table_object::table>,
                    bmi::member<cache_table_object, const field_name,   &cache_table_object::pk_field>>>>>;

    struct cache_map::cache_map_impl_ final {
        cache_map_impl_() = default;
        ~cache_map_impl_() = default;

        cache_table_object* find(const table_info& table) {
            auto itr = tables.find(std::make_tuple(table.code, table.scope, table.table->name, table.pk_order->field));
            if (tables.end() != itr) return &const_cast<cache_table_object&>(*itr);

            return nullptr;
        }

        cache_table_object& get(const table_info& table) {
            auto cache = find(table);
            if (cache) return *cache;

            auto result = tables.emplace(table);
            return const_cast<cache_table_object&>(*result.first);
        }

        cache_table_index tables;
    }; // struct cache_map::cache_map_impl_

    cache_map::cache_map()
    : impl_(new cache_map_impl_())
    { }

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
            cache.map.emplace(item->pk, std::move(item));
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