#include "genesis_accounts.hpp"
#include <eosio/chain/controller.hpp>

namespace cyberway { namespace genesis {

using namespace eosio::chain;
using namespace cyberway::chaindb;


abi_def golos_vesting_contract_abi(abi_def abi = abi_def()) {
    set_common_defs(abi);
    abi.structs.emplace_back(struct_def{"user_balance", "", {
        {"vesting", "asset"},
        {"delegate_vesting", "asset"},
        {"received_vesting", "asset"},
        {"unlocked_limit", "asset"}}
    });
    abi.structs.emplace_back(struct_def{"balance_vesting", "", {  // TODO: rename…
        {"supply", "asset"},
        {"notify_acc", "name"}}
    });

    abi.tables.emplace_back(table_def{"accounts", "user_balance", {
        {"primary", true, {{"vesting.sym", "asc"}}}
    }});
    abi.tables.emplace_back(table_def{"vesting", "balance_vesting", {
        {"primary", true, {{"supply.sym", "asc"}}}
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

void init_genesis_accounts(chaindb_controller& chaindb) {//}, const genesis_state& conf, authorization_manager& am) {
    chaindb.add_abi(config::token_account_name, token_contract_abi());   // need to add here again
    chaindb.add_abi(gls_vest_account_name, golos_vesting_contract_abi());
    chaindb.add_abi(gls_ctrl_account_name, golos_control_contract_abi());
}

}} // cyberway::genesis
