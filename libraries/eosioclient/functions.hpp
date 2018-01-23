#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string>

namespace eosio {
namespace client {

const std::string chain_func_base = "/v1/chain";
const std::string get_info_func = chain_func_base + "/get_info";
const std::string push_txn_func = chain_func_base + "/push_transaction";
const std::string push_txns_func = chain_func_base + "/push_transactions";
const std::string json_to_bin_func = chain_func_base + "/abi_json_to_bin";
const std::string get_block_func = chain_func_base + "/get_block";
const std::string get_account_func = chain_func_base + "/get_account";
const std::string get_table_func = chain_func_base + "/get_table_rows";
const std::string get_code_func = chain_func_base + "/get_code";
const std::string get_required_keys = chain_func_base + "/get_required_keys";

const std::string account_history_func_base = "/v1/account_history";
const std::string get_transaction_func = account_history_func_base + "/get_transaction";
const std::string get_transactions_func = account_history_func_base + "/get_transactions";
const std::string get_key_accounts_func = account_history_func_base + "/get_key_accounts";
const std::string get_controlled_accounts_func = account_history_func_base + "/get_controlled_accounts";

const std::string net_func_base = "/v1/net";
const std::string net_connect = net_func_base + "/connect";
const std::string net_disconnect = net_func_base + "/disconnect";
const std::string net_status = net_func_base + "/status";
const std::string net_connections = net_func_base + "/connections";

} // namespace client
} // namespace eosio

#endif // FUNCTIONS_H
