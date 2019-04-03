#pragma once

#include <cyberway/chaindb/cache_item.hpp>

namespace cyberway { namespace chaindb {

    struct table_info;

    class cache_map final {
    public:
        cache_map(abi_map&);
        ~cache_map();

        void set_cache_converter(const table_info&, const cache_converter_interface&) const;

        void set_next_pk(const table_info&, primary_key_t) const;

        cache_object_ptr create(const table_info&) const;
        cache_object_ptr find(const table_info&, primary_key_t) const;

        cache_object_ptr emplace(const table_info&, object_value) const;
        void remove(const table_info&, primary_key_t) const;
        void set_revision(const object_value&, revision_t) const;

        void clear();

    private:
        std::unique_ptr<table_cache_map> impl_;
    }; // class cache_map

} } // namespace cyberway::chaindb
