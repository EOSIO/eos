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
        abi_info(const account_name& code, blob);

        void verify_tables_structure(const driver_interface&) const;

        variant to_object(const table_info&, const void*, size_t) const;
        variant to_object(const index_info&, const void*, size_t) const;
        variant to_object(const string&, const void*, size_t) const;
        bytes to_bytes(const table_info&, const variant&) const;
        bytes to_bytes(const index_info& info, const variant& value) const;

        string get_event_type(const event_name& n) const {
            return serializer_.get_event_type(n);
        }

        string get_action_type(const event_name& n) const {
            return serializer_.get_action_type(n);
        }

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

        const abi_serializer& serializer() const {
            return serializer_;
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

        void init(abi_def);

        template<typename Type> variant to_object_(abi_serializer::mode, const char*, Type&&, const string&, const void*, size_t) const;
        template<typename Type> bytes to_bytes_(const char*, Type&&, const string&, const variant&) const;
    }; // struct abi_info

    //-------------------------------------------------------------------

    abi_def merge_abi_def(abi_def, abi_def);
    bytes   merge_abi_def(abi_def, const bytes&);
    bytes   merge_abi_def(const bytes&, const bytes&);


} } // namespace cyberway::chaindb