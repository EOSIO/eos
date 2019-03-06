#pragma once

#include <cyberway/chaindb/cache_item.hpp>

namespace cyberway { namespace chaindb {

    struct table_info;

    class cache_map final {
    public:
        cache_map();
        ~cache_map();

        void set_cache_converter(const table_info&, const cache_converter_interface&) const;

        void set_next_pk(const table_info&, primary_key_t) const;

        cache_item_ptr create(const table_info&) const;
        cache_item_ptr find(const table_info&, primary_key_t) const;

        cache_item_ptr emplace(const table_info&, object_value) const;
        void remove(const table_info&, primary_key_t) const;

        void clear();

    private:
        struct cache_map_impl_;
        std::unique_ptr<cache_map_impl_> impl_;
    }; // class cache_map

} } // namespace cyberway::chaindb
