#pragma once
#include <eosio/chain/types.hpp>
#include <fc/reflect/reflect.hpp>
#include <boost/filesystem.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bfs = boost::filesystem;

struct genesis_info {

    struct file_hash {
        bfs::path path;
        fc::sha256 hash;
    };

    struct account {
        account_name name;
        std::string owner_key;      // use string here to allow "INITIAL" keyword
        std::string active_key;
        fc::optional<file_hash> abi;
        fc::optional<file_hash> code;
    };

    bfs::path state_file;   // ? file_hash
    bfs::path genesis_json;
    std::vector<account> accounts;
};

}} // cyberway::genesis

FC_REFLECT(cyberway::genesis::genesis_info::file_hash, (path)(hash))
FC_REFLECT(cyberway::genesis::genesis_info::account, (name)(owner_key)(active_key)(abi)(code))
FC_REFLECT(cyberway::genesis::genesis_info, (state_file)(genesis_json)(accounts))
