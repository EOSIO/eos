#pragma once
#include <eosio/chain/types.hpp>
#include <cyberway/chaindb/common.hpp>
#include <fc/reflect/reflect.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
using chaindb::primary_key_t;


struct genesis_header {
    char magic[12] = "CyberwayGen";
    uint32_t version = 1;

    uint32_t tables_count;

    bool is_valid() {
        genesis_header oth;
        return string(magic) == oth.magic && version == oth.version;
    }
};

struct table_header {
    account_name code;
    table_name name;
    type_name abi_type;
    uint32_t count;
};

struct sys_table_row {
    account_name ram_payer;
    bytes data;
};

struct table_row: sys_table_row {
    table_row(sys_table_row base, primary_key_t pk, uint64_t scope): sys_table_row(base), pk(pk), scope(scope) {
    }

    primary_key_t pk;
    uint64_t scope;
};


}} // cyberway::genesis

FC_REFLECT(cyberway::genesis::table_header, (code)(name)(abi_type)(count))
FC_REFLECT(cyberway::genesis::sys_table_row, (ram_payer)(data))
FC_REFLECT_DERIVED(cyberway::genesis::table_row, (cyberway::genesis::sys_table_row), (pk)(scope))
