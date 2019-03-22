#include "genesis_accounts.hpp"
#include <eosio/chain/controller.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
using namespace cyberway::chaindb;


abi_def golos_vesting_contract_abi(abi_def abi = abi_def()) {
    set_common_defs(abi);
    abi.structs.emplace_back(struct_def{"account", "", {
        {"vesting", "asset"},
        {"delegated", "asset"},
        {"received", "asset"},
        {"unlocked_limit", "asset"}}
    });
    abi.structs.emplace_back(struct_def{"vesting_stats", "", {
        {"supply", "asset"},
        {"notify_acc", "name"}}
    });
    abi.structs.emplace_back(struct_def{"delegation", "", {
        {"id", "uint64"},
        {"delegator", "name"},
        {"delegatee", "name"},
        {"quantity", "asset"},
        {"interest_rate", "uint16"},
        {"payout_strategy", "uint8"},
        {"min_delegation_time", "time_point_sec"}}
    });
    abi.structs.emplace_back(struct_def{"return_delegation", "", {
        {"id", "uint64"},
        {"delegator", "name"},
        {"quantity", "asset"},
        {"date", "time_point_sec"}}
    });

    abi.tables.emplace_back(table_def{"accounts", "account", {
        {"primary", true, {{"vesting.sym", "asc"}}}
    }});
    abi.tables.emplace_back(table_def{"stat", "vesting_stats", {
        {"primary", true, {{"supply.sym", "asc"}}}
    }});
    abi.tables.emplace_back(table_def{"delegation", "delegation", {
        {"primary", true, {{"id", "asc"}}},
        {"delegator",  true, {{"delegator", "asc"}, {"delegatee", "asc"}}},
        {"delegatee",  false, {{"delegatee", "asc"}}}
    }});
    abi.tables.emplace_back(table_def{"rdelegation", "return_delegation", {
        {"primary", true, {{"id", "asc"}}},
        {"date", false, {{"date", "asc"}}}
    }});
    return abi;
}

abi_def golos_control_contract_abi(abi_def abi = abi_def()) {
    set_common_defs(abi);
    abi.structs.emplace_back(struct_def{"witness_info", "", {
        {"name",  "name"},
        {"url",   "string"},
        {"active","bool"},
        {"total_weight", "uint64"}
    }});
    abi.structs.emplace_back(struct_def{"witness_voter", "", {
        {"voter", "name"},
        {"witnesses", "name[]"}
    }});

    abi.tables.emplace_back(table_def{"witness", "witness_info", {
        {"primary", true, {{"name", "asc"}}},
        {"byweight", false, {{"total_weight", "desc"}}}
    }});
    abi.tables.emplace_back(table_def{"witnessvote", "witness_voter", {
        {"primary", true, {{"voter", "asc"}}}
    }});
    return abi;
}

void init_genesis_accounts(chaindb_controller& chaindb) {
    chaindb.add_abi(config::token_account_name, token_contract_abi());   // need to add here again
    chaindb.add_abi(gls_vest_account_name, golos_vesting_contract_abi());
    chaindb.add_abi(gls_ctrl_account_name, golos_control_contract_abi());
}

}} // cyberway::genesis
