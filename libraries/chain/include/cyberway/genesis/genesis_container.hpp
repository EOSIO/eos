#pragma once
#include <eosio/chain/types.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <fc/reflect/reflect.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
using namespace chaindb;


struct genesis_header {
    char magic[12] = "CyberwayGen";
    uint32_t version = 1;

    uint32_t tables_count;

    bool is_valid() const {
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

    sys_table_row() = default;
    sys_table_row(account_name payer, bytes data)
    :   ram_payer(payer)
    ,   data(std::move(data)) {
    }

    table_request request(table_name t) const {
        return table_request{
            .code = name(),
            .scope = name(),
            .table = t
        };
    }
};

struct table_row final: sys_table_row {
    table_row() = default;
    table_row(account_name payer, bytes data, primary_key_t pk, uint64_t scope)
    :   sys_table_row(payer, std::move(data))
    ,   pk(pk)
    ,   scope(scope) {
    }

    primary_key_t pk;
    uint64_t scope;

    table_request request(account_name a, table_name t) const {
        return table_request{
            .code = a,
            .scope = scope,
            .table = t
        };
    }
};


}} // cyberway::genesis

FC_REFLECT(cyberway::genesis::table_header, (code)(name)(abi_type)(count))
FC_REFLECT(cyberway::genesis::sys_table_row, (ram_payer)(data))
FC_REFLECT_DERIVED(cyberway::genesis::table_row, (cyberway::genesis::sys_table_row), (pk)(scope))
