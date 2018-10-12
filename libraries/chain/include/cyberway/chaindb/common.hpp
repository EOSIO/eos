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

    std::ostream& operator<<(std::ostream&, const chaindb_type);
    std::istream& operator>>(std::istream&, chaindb_type&);

    using std::string;

    using eosio::chain::account_name;
    using eosio::chain::table_def;
    using eosio::chain::index_def;

    using table_name_t = eosio::chain::table_name::value_type;
    using index_name_t = eosio::chain::index_name::value_type;
    using account_name_t = account_name::value_type;

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM( cyberway::chaindb::chaindb_type, (MongoDB) )