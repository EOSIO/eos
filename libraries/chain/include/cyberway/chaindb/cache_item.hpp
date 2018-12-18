#pragma once

#include <memory>

#include <fc/variant.hpp>

#include <cyberway/chaindb/common.hpp>

namespace cyberway { namespace chaindb {
    using fc::variant;

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

        virtual cache_item_data_ptr convert_variant(cache_item&, primary_key_t, const variant&) const = 0;
    }; // struct cache_load_interface

    class cache_item final {
        const cache_converter_interface& converter_;
        bool is_deleted_ = false;

    public:
        cache_item(const primary_key_t pk, const cache_converter_interface& converter)
        : converter_(converter),
          pk(pk)
        { }

        ~cache_item() = default;

        bool is_deleted() const {
            return is_deleted_;
        }

        void mark_deleted() {
            assert(!is_deleted_);
            is_deleted_ = true;
        }

        void convert_variant(const variant& value) {
            data = converter_.convert_variant(*this, pk, value);
        }

        bool is_valid_converter(const cache_converter_interface& value) const {
            return &converter_ == &value;
        }

        const primary_key_t pk;
        cache_item_data_ptr data;
    }; // class cache_item

} } // namespace cyberway::chaindb
