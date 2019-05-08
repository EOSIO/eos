#pragma once

#include <string>

#include <eosio/chain/types.hpp>
#include <eosio/chain/abi_def.hpp>

namespace cyberway { namespace chaindb {

    using revision_t = int64_t;
    static constexpr revision_t impossible_revision = (-1);
    static constexpr revision_t unset_revision = (-2);
    static constexpr revision_t start_revision = (1);

    using primary_key_t = uint64_t;
    static constexpr primary_key_t unset_primary_key = (-2);
    static constexpr primary_key_t end_primary_key = (-1);

    using cursor_t = int32_t;
    static constexpr cursor_t invalid_cursor = (0);

    using std::string;

    using eosio::chain::account_name;
    using eosio::chain::table_name;
    using eosio::chain::index_name;
    using eosio::chain::table_def;
    using eosio::chain::index_def;
    using eosio::chain::order_def;
    using eosio::chain::field_name;

    using table_name_t = table_name::value_type;
    using index_name_t = index_name::value_type;
    using account_name_t = account_name::value_type;

    struct table_request final {
        const account_name code;
        const account_name scope;
        const table_name_t table;
    }; // struct table_request

    struct index_request final {
        const account_name code;
        const account_name scope;
        const table_name_t table;
        const index_name_t index;

        operator table_request() const {
            return {code, scope, table};
        }
    }; // struct index_request

    struct cursor_request final {
        const account_name code;
        const cursor_t     id;
    }; // struct cursor_request

    enum class chaindb_type {
        MongoDB,
        // TODO: RocksDB
    };

    class chaindb_controller;
    class abi_info;

    using abi_map = std::map<account_name /* code */, abi_info>;

    struct table_info {
        const account_name code;
        const account_name scope;
        const table_def*   table    = nullptr;
        const order_def*   pk_order = nullptr;
        const abi_info*    abi      = nullptr;

        table_info(const account_name& code, const account_name& scope)
        : code(code), scope(scope) {
        }
    }; // struct table_info

    struct index_info: public table_info {
        const index_def* index = nullptr;

        using table_info::table_info;

        index_info(const table_info& src)
        : table_info(src) {
        }
    }; // struct index_info

    struct find_info final {
        cursor_t      cursor = invalid_cursor;
        primary_key_t pk     = end_primary_key;
    }; // struct find_info

    class chaindb_session final {
    public:
        chaindb_session(chaindb_session&& mv);
        ~chaindb_session();

        chaindb_session& operator=(chaindb_session&& mv) = delete;

        void apply_changes();

        /** leaves the UNDO state on the stack when session goes out of scope */
        void push();

        /** Combines this session with the prior session */
        void squash();

        /** Undo changes made in this session */
        void undo();

        uint64_t calc_ram_bytes() const;

        revision_t revision() const {
            return revision_;
        }

    private:
        friend class chaindb_controller;

        chaindb_session(chaindb_controller&, revision_t);

        chaindb_controller& controller_;
        bool apply_ = true;
        const revision_t revision_ = -1;
    }; // struct session

    std::ostream& operator<<(std::ostream&, const chaindb_type);
    std::istream& operator>>(std::istream&, chaindb_type&);

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM( cyberway::chaindb::chaindb_type, (MongoDB) )
