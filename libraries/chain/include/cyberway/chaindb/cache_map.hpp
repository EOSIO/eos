#pragma once

#include <cyberway/chaindb/cache_item.hpp>

namespace cyberway { namespace chaindb {

    struct table_info;

    class cache_map final {
    public:
        cache_map();
        ~cache_map();

        primary_key_t get_next_pk(const table_info&) const;
        void set_next_pk(const table_info&, primary_key_t) const;

        cache_item_ptr find(const table_info&, primary_key_t);

        void insert(const table_info&, cache_item_ptr);
        void update(const table_info&, primary_key_t, const variant&);
        void remove(const table_info&, primary_key_t);

    private:
        struct cache_map_impl_;
        std::unique_ptr<cache_map_impl_> impl_;
    }; // class cache_map

} } // namespace cyberway::chaindb
