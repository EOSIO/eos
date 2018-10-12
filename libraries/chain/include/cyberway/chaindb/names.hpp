#pragma once

#include <cyberway/chaindb/common.hpp>

namespace cyberway { namespace chaindb {

    const string& system_code_name();
    const string& unknown_name();
    const string& scope_field_name();
    const string& payer_field_name();
    const string& size_field_name();

    ///----

    inline string code_name(const account_name& code) {
        if (!code.empty()) {
            return code.to_string();
        }
        return system_code_name();
    }

    inline string code_name(const account_name_t code) {
        return code_name(account_name(code));
    }

    template <typename Info>
    inline string code_name(const Info& info) {
        return code_name(info.code);
    }

    ///----

    inline string scope_name(const account_name& scope) {
        return code_name(scope);
    }

    template <typename Info>
    inline string scope_name(const Info& info) {
        return scope_name(info.scope);
    }

    ///---

    inline string payer_name(const account_name& payer) {
        return payer.to_string();
    }

    ///----

    inline string table_name(const eosio::chain::table_name& table) {
        if (!table.empty()) {
            return table.to_string();
        }
        return unknown_name();
    }

    inline string table_name(const table_name_t table) {
        return table_name(eosio::chain::table_name(table));
    }

    inline string table_name(const table_def* table) {
        if (table) {
            return table_name(table->name);
        }
        return unknown_name();
    }

    template <typename Info>
    inline string table_name(const Info& info) {
        return table_name(info.table);
    }

    template<typename Info>
    inline string full_table_name(const Info& info) {
        return code_name(info).append(".").append(table_name(info));
    }

    ///-----

    inline string index_name(const eosio::chain::index_name& index) {
        if (!index.empty()) {
            return index.to_string();
        }
        return unknown_name();
    }

    inline string index_name(const index_name_t index) {
        return index_name(eosio::chain::index_name(index));
    }

    template <typename Info>
    inline string index_name(const Info& info) {
        return index_name(info.index);
    }

    inline string index_name(const index_def& index) {
        return index_name(index.name);
    }

    inline string index_name(const index_def* index) {
        if (index) {
            return index_name(*index);
        }
        return unknown_name();
    }

    template<typename Info>
    inline string full_index_name(const Info& info) {
        return full_table_name(info).append(".").append(index_name(info.index));
    }
} } // namespace cyberway::chaindb