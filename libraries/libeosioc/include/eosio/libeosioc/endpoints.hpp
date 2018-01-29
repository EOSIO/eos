/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <string>

namespace eosio {
namespace client {

const std::string chain_base_endpoint = "/v1/chain";
const std::string get_info_endpoint = chain_base_endpoint + "/get_info";
const std::string push_txn_endpoint = chain_base_endpoint + "/push_transaction";
const std::string push_txns_endpoint = chain_base_endpoint + "/push_transactions";
const std::string json_to_bin_endpoint = chain_base_endpoint + "/abi_json_to_bin";
const std::string get_block_endpoint = chain_base_endpoint + "/get_block";
const std::string get_account_endpoint = chain_base_endpoint + "/get_account";
const std::string get_table_endpoint = chain_base_endpoint + "/get_table_rows";
const std::string get_code_endpoint = chain_base_endpoint + "/get_code";
const std::string get_currency_balance_endpoint = chain_base_endpoint + "/get_currency_balance";
const std::string get_currency_stats_endpoint = chain_base_endpoint + "/get_currency_stats";
const std::string get_required_keys = chain_base_endpoint + "/get_required_keys";

const std::string account_history_base_endpoint = "/v1/account_history";
const std::string get_transaction_endpoint = account_history_base_endpoint + "/get_transaction";
const std::string get_transactions_endpoint = account_history_base_endpoint + "/get_transactions";
const std::string get_key_accounts_endpoint = account_history_base_endpoint + "/get_key_accounts";
const std::string get_controlled_accounts_endpoint = account_history_base_endpoint + "/get_controlled_accounts";

const std::string net_base_endpoint = "/v1/net";
const std::string net_connect = net_base_endpoint + "/connect";
const std::string net_disconnect = net_base_endpoint + "/disconnect";
const std::string net_status = net_base_endpoint + "/status";
const std::string net_connections = net_base_endpoint + "/connections";

const std::string wallet_base_endpoint = "/v1/wallet";
const std::string wallet_create_endpoint = wallet_base_endpoint + "/create";
const std::string wallet_open_endpoint = wallet_base_endpoint + "/open";
const std::string wallet_list_endpoint = wallet_base_endpoint + "/list_wallets";
const std::string wallet_list_keys_endpoint = wallet_base_endpoint + "/list_keys";
const std::string wallet_public_keys_endpoint = wallet_base_endpoint + "/get_public_keys";
const std::string wallet_lock_endpoint = wallet_base_endpoint + "/lock";
const std::string wallet_lock_all_endpoint = wallet_base_endpoint + "/lock_all";
const std::string wallet_unlock_endpoint = wallet_base_endpoint + "/unlock";
const std::string wallet_import_key_endpoint = wallet_base_endpoint + "/import_key";
const std::string wallet_sign_trx_endpoint = wallet_base_endpoint + "/sign_transaction";

} // namespace client
} // namespace eosio

