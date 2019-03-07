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

    enum class chaindb_type {
        MongoDB,
        // TODO: RocksDB
    };

    class chaindb_controller;
    class undo_stack;
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

        revision_t revision() const {
            return revision_;
        }

    private:
        friend class undo_stack;

        chaindb_session(undo_stack& stack, revision_t revision);

        undo_stack& stack_;
        bool apply_ = true;
        const revision_t revision_ = -1;
    }; // struct session

    std::ostream& operator<<(std::ostream&, const chaindb_type);
    std::istream& operator>>(std::istream&, chaindb_type&);

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM( cyberway::chaindb::chaindb_type, (MongoDB) )
