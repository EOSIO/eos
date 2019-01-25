#pragma once

#include <map>

#include <eosio/chain/abi_serializer.hpp>

#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/driver_interface.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::abi_serializer;
    using eosio::chain::bytes;

    using fc::time_point;
    using fc::variant;

    template <typename Info>
    const index_def& get_pk_index(const Info& info) {
        // abi structure is already validated by abi_serializer
        return info.table->indexes.front();
    }

    template <typename Info>
    const order_def& get_pk_order(const Info& info) {
        // abi structure is already validated by abi_serializer
        return get_pk_index(info).orders.front();
    }

    class abi_info final {
    public:
        abi_info() = default;
        abi_info(const account_name& code, abi_def);

        void verify_tables_structure(driver_interface&) const;

        variant to_object(const table_info& info, const void* data, const size_t size) const {
            CYBERWAY_ASSERT(info.table, unknown_table_exception, "NULL table");
            return to_object_("table", [&]{return get_full_table_name(info);}, info.table->type, data, size);
        }

        variant to_object(const index_info& info, const void* data, const size_t size) const {
            CYBERWAY_ASSERT(info.index, unknown_index_exception, "NULL index");
            auto type = get_full_index_name(info);
            return to_object_("index", [&](){return type;}, type, data, size);
        }

        bytes to_bytes(const table_info& info, const variant& value) const {
            CYBERWAY_ASSERT(info.table, unknown_table_exception, "NULL table");
            return to_bytes_("table", [&]{return get_full_table_name(info);}, info.table->type, value);
        }

        void mark_removed() {
            is_removed_ = true;
        }

        bool is_removed() const {
            return is_removed_;
        }

        const table_def* find_table(hash_t hash) const {
            auto itr = table_map_.find(hash);
            if (table_map_.end() != itr) return &itr->second;

            return nullptr;
        }

        const account_name& code() const {
            return code_;
        }

        static const size_t max_table_cnt;
        static const size_t max_index_cnt;

    private:
        const account_name code_;
        eosio::chain::abi_serializer serializer_;
        std::map<hash_t, table_def> table_map_;
        bool is_removed_ = false;
        static const fc::microseconds max_abi_time_;

        template<typename Type>
        variant to_object_(
            const char* value_type, Type&& db_type, const string& type, const void* data, const size_t size
        ) const {
            // begin()
            if (nullptr == data || 0 == size) return fc::variant_object();

            fc::datastream<const char*> ds(static_cast<const char*>(data), size);
            auto value = serializer_.binary_to_variant(type, ds, max_abi_time_);

//            dlog(
//                "The ${value_type} '${type}': ${value}",
//                ("value_type", value_type)("type", db_type())("value", value));

            CYBERWAY_ASSERT(value.is_object(), invalid_abi_store_type_exception,
                "ABI serializer returns bad type for the ${value_type} for ${type}: ${value}",
                ("value_type", value_type)("type", db_type())("value", value));

            return value;
        }

        template<typename Type>
        bytes to_bytes_(const char* value_type, Type&& db_type, const string& type, const variant& value) const {

//            dlog(
//                "The ${value_type} '${type}': ${value}",
//                ("value", value_type)("type", db_type())("value", value));

            CYBERWAY_ASSERT(value.is_object(), invalid_abi_store_type_exception,
                "ABI serializer receive wrong type for the ${value_type} for '${type}': ${value}",
                ("value_type", value_type)("type", db_type())("value", value));

            return serializer_.variant_to_binary(type, value, max_abi_time_);
        }
    }; // struct abi_info

} } // namespace cyberway::chaindb