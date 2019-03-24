#pragma once

#include <memory>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/object_value.hpp>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>

namespace cyberway { namespace chaindb {

    class  cache_item;
    struct cache_item_data;
    struct cache_converter_interface;

    using cache_item_ptr = boost::intrusive_ptr<cache_item>;
    using cache_item_data_ptr = std::unique_ptr<cache_item_data>;

    struct cache_item_data {
        virtual ~cache_item_data() = default;
    }; // struct cache_item_data

    struct cache_converter_interface {
        cache_converter_interface();
        virtual ~cache_converter_interface();

        virtual cache_item_data_ptr convert_variant(cache_item&, const object_value&) const = 0;
    }; // struct cache_load_interface

    struct table_cache_map;

    class cache_item final:
        public boost::intrusive_ref_counter<cache_item>,
        public boost::intrusive::set_base_hook<>,
        public boost::intrusive::list_base_hook<>
    {
        table_cache_map* table_cache_map_ = nullptr;
        object_value     object_;

    public:
        cache_item(table_cache_map& map, object_value obj)
        : table_cache_map_(&map), object_(std::move(obj)) {
        }

        cache_item(cache_item&&) = default;

        ~cache_item() = default;

        bool is_deleted() const {
            return nullptr != table_cache_map_;
        }

        void mark_deleted();

        template <typename Request>
        bool is_valid_table(const Request& request) const {
            return
                object_.service.code  == request.code  &&
                object_.service.scope == request.scope &&
                object_.service.table == request.table;
        }

        primary_key_t pk() const {
            return object_.pk();
        }

        void set_object(object_value obj) {
            assert(is_valid_table(obj.service));
            object_ = std::move(obj);
        }

        void set_revision(const revision_t rev) {
            object_.service.revision = rev;
        }

        const object_value& object() const {
            return object_;
        }

        cache_item_data_ptr data;
    }; // class cache_item

} } // namespace cyberway::chaindb
