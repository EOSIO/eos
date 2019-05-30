#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/abi_def.hpp>
#include <fc/reflect/reflect.hpp>

namespace eosio {

using namespace eosio::chain;

struct ee_genesis_header {
    char magic[12] = "CyberwayEEG";
    uint32_t version = 1;
    fc::sha256 hash;

    bool is_valid() const {
        ee_genesis_header oth;
        return string(magic) == oth.magic && version == oth.version;
    }
};

struct ee_table_header {
    account_name code;
    table_name   name;
    type_name    abi_type;
    uint32_t     count = 0;
};

} // eosio

FC_REFLECT(eosio::ee_table_header, (code)(name)(abi_type)(count))
