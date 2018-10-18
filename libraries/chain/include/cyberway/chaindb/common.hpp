#pragma once

#include <string>

#include <eosio/chain/types.hpp>
#include <eosio/chain/abi_def.hpp>

namespace cyberway { namespace chaindb {

    enum class chaindb_type {
        MongoDB,
        // TODO: RocksDB
    };

    class chaindb_controller;
    class undo_stack;

    class session final {
    public:
        session(session&& mv);
        ~session();

        session& operator=(session&& mv) = delete;

        /** leaves the UNDO state on the stack when session goes out of scope */
        void push();

        /** Combines this session with the prior session */
        void squash();

        /** Undo changes made in this session */
        void undo();

        int64_t revision() const {
            return revision_;
        }

    private:
        friend class undo_stack;

        session(undo_stack& stack, int64_t revision);

        undo_stack& stack_;
        bool apply_ = true;
        const int64_t revision_ = -1;
    }; // struct session

    std::ostream& operator<<(std::ostream&, const chaindb_type);
    std::istream& operator>>(std::istream&, chaindb_type&);

    using std::string;

    using eosio::chain::account_name;
    using eosio::chain::table_name;
    using eosio::chain::index_name;
    using eosio::chain::table_def;
    using eosio::chain::index_def;

    using table_name_t = table_name::value_type;
    using index_name_t = index_name::value_type;
    using account_name_t = account_name::value_type;

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM( cyberway::chaindb::chaindb_type, (MongoDB) )