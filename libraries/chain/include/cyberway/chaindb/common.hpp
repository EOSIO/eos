#pragma once

#include <eosio/chain/types.hpp>

namespace cyberway { namespace chaindb {

    enum class chaindb_type {
        MongoDB,
        // TODO: RocksDB
    };

    class chaindb_controller;

    std::ostream& operator<<(std::ostream&, const chaindb_type);
    std::istream& operator>>(std::istream&, chaindb_type&);

} } // namespace cyberway::chaindb

FC_REFLECT_ENUM( cyberway::chaindb::chaindb_type, (MongoDB) )