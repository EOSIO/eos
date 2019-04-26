#pragma once

#include <cyberway/chaindb/cache_item.hpp>

namespace cyberway { namespace chaindb {

    struct chaindb_cursor_cache final {
        account_name_t   cache_code   = 0;
        cursor_t         cache_cursor = invalid_cursor;

        cache_object_ptr cache;

        void reset() {
            cache_code   = 0;
            cache_cursor = invalid_cursor;
            cache.reset();
        }
    }; // struct chaindb_cursor_cache

} } //namespace cyberway::chaindb