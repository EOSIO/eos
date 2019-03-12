#pragma once

#include <cyberway/chaindb/common.hpp>
#include <algorithm>

namespace cyberway { namespace chaindb {

    struct names {
        static const string unknown;

        static const string system_code;

        static const string undo_table;

        static const string service_field;
        static const string code_field;
        static const string table_field;
        static const string scope_field;
        static const string pk_field;

        static const string next_pk_field;
        static const string undo_pk_field;
        static const string undo_rec_field;
        static const string undo_payer_field;
        static const string undo_size_field;
        static const string revision_field;

        static const string payer_field;
        static const string size_field;

        static const string scope_path;
        static const string undo_pk_path;
        static const string revision_path;

        static const string asc_order;
        static const string desc_order;

        static constexpr uint64_t primary_index = N(primary);
    }; // struct names


    ///----

    string db_name_to_string(const uint64_t&);

    inline account_name get_system_code() {
        return account_name();
    }

    inline bool is_system_code(const account_name& code) {
        return (code.empty());
    }

    inline string get_code_name(string name, const account_name& code) {
        if (!is_system_code(code)) name.append(db_name_to_string(code.value));
        return name;
    }

    inline string get_code_name(const account_name& code) {
        return get_code_name(names::system_code, code);
    }

    inline string get_code_name(const account_name_t code) {
        return get_code_name(account_name(code));
    }

    template <typename Info>
    inline string get_code_name(const Info& info) {
        return get_code_name(info.code);
    }

    ///----

    inline string get_scope_name(const account_name& scope) {
        return scope.to_string();
    }

    template <typename Info>
    inline string get_scope_name(const Info& info) {
        return get_scope_name(info.scope);
    }

    ///---

    inline string get_payer_name(const account_name& payer) {
        return payer.to_string();
    }

    ///----

    inline string get_table_name(const table_name& table) {
        if (!table.empty()) {
            return db_name_to_string(table.value);
        }
        return names::unknown;
    }

    inline string get_table_name(const table_name_t table) {
        return get_table_name(eosio::chain::table_name(table));
    }

    inline string get_table_name(const table_def& table) {
        return get_table_name(table.name);
    }

    inline string get_table_name(const table_def* table) {
        if (table) {
            return get_table_name(*table);
        }
        return names::unknown;
    }

    template <typename Info>
    inline string get_table_name(const Info& info) {
        return get_table_name(info.table);
    }

    template<typename Info>
    inline string get_full_table_name(const Info& info) {
        return get_code_name(info).append(".").append(get_table_name(info));
    }

    template<typename Code, typename Table>
    inline string get_full_table_name(const Code& code, const Table& table) {
        return get_code_name(code).append(".").append(get_table_name(table));
    }

    ///-----

    inline string get_index_name(const index_name& index) {
        if (!index.empty()) {
            return db_name_to_string(index.value);
        }
        return names::unknown;
    }

    inline string get_index_name(const index_name_t index) {
        return get_index_name(eosio::chain::index_name(index));
    }

    inline string get_index_name(const index_def& index) {
        return get_index_name(index.name);
    }

    inline string get_index_name(const index_def* index) {
        if (index) {
            return get_index_name(*index);
        }
        return names::unknown;
    }

    template <typename Info>
    inline string get_index_name(const Info& info) {
        return get_index_name(info.index);
    }

    template<typename Info>
    inline string get_full_index_name(const Info& info) {
        return get_full_table_name(info).append(".").append(get_index_name(info.index));
    }

    template<typename Table, typename Index>
    inline string get_full_index_name(const Table& table, const Index& index) {
        return get_full_table_name(table).append(".").append(get_index_name(index));
    }

    template<typename Code, typename Table, typename Index>
    inline string get_full_index_name(const Code& code, const Table& table, const Index& index) {
        return get_full_table_name(code, table).append(".").append(get_index_name(index));
    }

} } // namespace cyberway::chaindb
