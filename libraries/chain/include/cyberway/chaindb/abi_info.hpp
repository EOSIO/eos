#pragma once

#include <map>

#include <eosio/chain/abi_serializer.hpp>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/names.hpp>

namespace cyberway { namespace chaindb {

    using eosio::chain::abi_def;
    using eosio::chain::abi_serializer;

    struct abi_info final {
        abi_info() = default;
        abi_info(const account_name& code, abi_def);

        void verify_tables_structure(const driver_interface&) const;

        variant to_object(const table_info&, const void*, size_t) const;
        variant to_object(const index_info&, const void*, size_t) const;
        bytes to_bytes(const table_info&, const variant&) const;
        bytes to_bytes(const index_info& info, const variant& value) const;

        const table_def* find_table(table_name_t table) const {
            auto itr = table_map_.find(table);
            if (table_map_.end() != itr) {
                return &itr->second;
            }

            return nullptr;
        }

        const index_def* find_index(const table_def& table, const index_name_t index) const {
            for (auto& idx: table.indexes) if (index == idx.name.value) {
                return &idx;
            }
            return nullptr;
        }

        const index_def* find_pk_index(const table_def& table) const {
            if (!table.indexes.empty()) {
                return &table.indexes.front();
            }
            return nullptr;
        }

        const order_def* find_pk_order(const table_def& table) const {
            if (!table.indexes.empty()) {
                return &table.indexes.front().orders.front();
            }
            return nullptr;
        }

        const account_name& code() const {
            return code_;
        }

        enum : size_t {
            MaxTableCnt  = 64,
            MaxIndexCnt  = 16,
            MaxPathDepth = 4,
        }; // constants

    private:
        const account_name code_;
        abi_serializer serializer_;
        fc::flat_map<table_name_t, table_def> table_map_;
        static const fc::microseconds max_abi_time_;

        template<typename Type>
        variant to_object_(
            const char* value_type, Type&& db_type, const string& type, const void* data, const size_t size
        ) const {
            // begin()
            if (nullptr == data || 0 == size) return fc::variant_object();

            fc::datastream<const char*> ds(static_cast<const char*>(data), size);
            auto value = serializer_.binary_to_variant(type, ds, max_abi_time_, abi_serializer::DBMode);

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

    //-------------------------------------------------------------------

    abi_def merge_abi_def(abi_def, abi_def);
    bytes   merge_abi_def(abi_def, const bytes&);
    bytes   merge_abi_def(const bytes&, const bytes&);


} } // namespace cyberway::chaindb