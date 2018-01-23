#include "peer.hpp"

#include "functions.hpp"

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
    return m_host;
}

void Peer::set_host(const std::string &host)
{
    m_host = host;
}

uint32_t Peer::port() const
{
    return m_port;
}

void Peer::set_port(uint32_t port)
{
    m_port = port;
}

fc::variant Peer::get_info() const
{
    return m_remote.call(m_host, m_port, get_info_func);
}

fc::variant Peer::get_code(const std::string &account_name) const
{
    return m_remote.call(m_host, m_port, get_code_func, fc::mutable_variant_object("account_name", account_name));
}

fc::variant Peer::get_table(const std::string& scope, const std::string& code, const std::string& table) const
{
    bool binary = false;
    return m_remote.call(m_host, m_port, get_table_func, fc::mutable_variant_object("json", !binary)
                ("scope",scope)
                ("code",code)
                ("table",table)
                       );
}

fc::variant Peer::push_transaction(const chain::signed_transaction &transaction) const
{
    return m_remote.call(m_host, m_port, push_txn_func, transaction);
}

fc::variant Peer::push_transaction(const fc::variant &transaction) const
{
    return m_remote.call(m_host, m_port, push_txn_func, transaction);
}

fc::variant Peer::connect(const std::string &host) const
{
    return m_remote.call(m_host, m_port, net_connect, host);
}

fc::variant Peer::disconnect(const std::string &host) const
{
    return m_remote.call(m_host, m_port, net_disconnect, host);
}

fc::variant Peer::status(const std::string &host) const
{
    return m_remote.call(m_host, m_port, net_status, host);
}

fc::variant Peer::connections(const std::string &host) const
{
    return m_remote.call(m_host, m_port, net_connections, host);
}

fc::variant Peer::get_account_function(const fc::mutable_variant_object& variant) const
{
    return m_remote.call(m_host, m_port, get_account_func, variant);
}

fc::variant Peer::get_block_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(m_host, m_port, get_block_func, variant);
}

fc::variant Peer::get_key_accounts_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(m_host, m_port, get_key_accounts_func, variant);
}

fc::variant Peer::get_controlled_accounts_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(m_host, m_port, get_controlled_accounts_func, variant);
}

fc::variant Peer::get_transaction_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(m_host, m_port, get_transaction_func, variant);
}

fc::variant Peer::get_transactions_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(m_host, m_port, get_transactions_func, variant);
}

fc::variant Peer::push_transactions(const std::vector<chain::signed_transaction> &transactions) const
{
    return m_remote.call(m_host, m_port, push_txns_func, transactions);
}

fc::variant Peer::json_to_bin_function(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(m_host, m_port, json_to_bin_func, variant);
}

fc::variant Peer::get_keys_required(const fc::mutable_variant_object &variant) const
{
    return m_remote.call(m_host, m_port, get_required_keys, variant);
}

} // namespace client
} // namespace eosio
