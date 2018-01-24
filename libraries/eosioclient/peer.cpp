#include "peer.hpp"

namespace {
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
}

namespace eosio {
namespace client {

std::string Peer::host() const
{
    return m_remote.host();
}

void Peer::set_host(const std::string &host)
{
    m_remote.set_host(host);
}

uint32_t Peer::port() const
{
    return m_remote.port();
}

void Peer::set_port(uint32_t port)
{
    m_remote.set_port(port);
}

fc::variant Peer::get_info() const
{
    return m_remote.call(get_info_func);
}

fc::variant Peer::get_code(const std::string &account_name) const
{
    return m_remote.call(get_code_func, fc::mutable_variant_object("account_name", account_name));
}

fc::variant Peer::get_table(const std::string& scope, const std::string& code, const std::string& table) const
{
    bool binary = false;
    return m_remote.call(get_table_func, fc::mutable_variant_object("json", !binary)
                ("scope",scope)
                ("code",code)
                ("table",table)
                       );
}

fc::variant Peer::push_transaction(const chain::signed_transaction &transaction) const
{
    return m_remote.call(push_txn_func, transaction);
}

fc::variant Peer::push_transaction(const fc::variant &transaction) const
{
    return m_remote.call(push_txn_func, transaction);
}

fc::variant Peer::connect(const std::string &host) const
{
    return m_remote.call(net_connect, host);
}

fc::variant Peer::disconnect(const std::string &host) const
{
    return m_remote.call(net_disconnect, host);
}

fc::variant Peer::status(const std::string &host) const
{
    return m_remote.call(net_status, host);
}

fc::variant Peer::connections(const std::string &host) const
{
    return m_remote.call(net_connections, host);
}

fc::variant Peer::get_account_function(const std::string &name) const
{
    auto arg = fc::mutable_variant_object("account_name", name);
    return m_remote.call(get_account_func, arg);
}

fc::variant Peer::get_block_function(const std::string &id_or_num) const
{
    auto arg = fc::mutable_variant_object("block_num_or_id", id_or_num);
    return m_remote.call(get_block_func, arg);
}

fc::variant Peer::get_key_accounts_function(const std::string &public_key) const
{
    auto arg = fc::mutable_variant_object( "public_key", public_key);
    return m_remote.call(get_key_accounts_func, arg);
}

fc::variant Peer::get_controlled_accounts_function(const std::string &account) const
{
    auto arg = fc::mutable_variant_object( "controlling_account", account);
    return m_remote.call(get_controlled_accounts_func, arg);
}

fc::variant Peer::get_transaction_function(const std::string &id) const
{
    auto arg = fc::mutable_variant_object( "transaction_id", id);
    return m_remote.call(get_transaction_func, arg);
}

fc::variant Peer::get_transactions_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(get_transactions_func, variant);
}

fc::variant Peer::push_transactions(const std::vector<chain::signed_transaction> &transactions) const
{
    return m_remote.call(push_txns_func, transactions);
}

fc::variant Peer::json_to_bin_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(json_to_bin_func, variant);
}

fc::variant Peer::get_keys_required(const chain::signed_transaction &transaction, const fc::variant &public_keys) const
{
    auto get_arg = fc::mutable_variant_object("transaction", transaction)("available_keys", public_keys);
    return m_remote.call(get_required_keys, get_arg);
}

} // namespace client
} // namespace eosio
