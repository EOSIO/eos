#pragma once

#include <memory>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/object_value.hpp>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>

namespace cyberway { namespace chaindb {

    class cache_item;
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

    class cache_item final: public boost::intrusive_ref_counter<cache_item> {
        table_name   table_ = 0;
        object_value object_;
        bool         is_deleted_ = false;

    public:
        cache_item(object_value obj)
        : table_(obj.service.table),
          object_(std::move(obj)) {
        }

        ~cache_item() = default;

        bool is_deleted() const {
            return is_deleted_;
        }

        void mark_deleted() {
            assert(!is_deleted_);
            is_deleted_ = true;
        }

        bool is_valid_table(const table_name table) const {
            return table_ == table;
        }

        primary_key_t pk() const {
            return object_.pk();
        }

        void set_object(object_value obj) {
            assert(is_valid_table(obj.service.table));
            object_ = std::move(obj);
        }

        const object_value& object() const {
            return object_;
        }

        cache_item_data_ptr data;
    }; // class cache_item

} } // namespace cyberway::chaindb
