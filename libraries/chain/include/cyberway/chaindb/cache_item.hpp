#pragma once

#include <memory>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/object_value.hpp>

namespace cyberway { namespace chaindb {

    class cache_item;
    struct cache_item_data;
    struct cache_converter_interface;

    using cache_item_ptr = std::shared_ptr<cache_item>;
    using cache_item_data_ptr = std::unique_ptr<cache_item_data>;

    struct cache_item_data {
        virtual ~cache_item_data() = default;
    }; // struct cache_item_data

    struct cache_converter_interface {
        cache_converter_interface();
        virtual ~cache_converter_interface();

        virtual cache_item_data_ptr convert_variant(cache_item&, const object_value&) const = 0;
    }; // struct cache_load_interface

    class cache_item final {
        hash_t hash_ = 0;
        object_value object_;
        bool is_deleted_ = false;

    public:
        cache_item(object_value obj)
        : hash_(obj.service.hash),
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

        bool is_valid_table(const hash_t hash) const {
            return hash_ == hash;
        }

        primary_key_t pk() const {
            return object_.pk();
        }

        void set_object(object_value obj) {
            assert(is_valid_table(obj.service.hash));
            object_ = std::move(obj);
        }

        const object_value& object() const {
            return object_;
        }

        cache_item_data_ptr data;
    }; // class cache_item

} } // namespace cyberway::chaindb
