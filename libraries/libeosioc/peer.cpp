#include "peer.hpp"

#include "fc/io/json.hpp"
#include "functions.hpp"

namespace eosio {
namespace client {

fc::variant Peer::get_info() const
{
    return call(get_info_endpoint);
}

fc::variant Peer::get_code(const std::string &account_name) const
{
    return call(get_code_endpoint, fc::mutable_variant_object("account_name", account_name));
}

fc::variant Peer::get_table(const std::string& scope, const std::string& code, const std::string& table) const
{
    bool binary = false;
    return call(get_table_endpoint, fc::mutable_variant_object("json", !binary)
                         ("scope",scope)
                         ("code",code)
                         ("table",table)
                         );
}

fc::variant Peer::push_transaction(const chain::signed_transaction &transaction) const
{
    return call(push_txn_endpoint, transaction);
}

fc::variant Peer::push_JSON_transaction(const fc::variant &transaction) const
{
    return call(push_txn_endpoint, transaction);
}

fc::variant Peer::connect_to(const std::string &host) const
{
    return call(net_connect, host);
}

fc::variant Peer::disconnect_from(const std::string &host) const
{
    return call(net_disconnect, host);
}

fc::variant Peer::status(const std::string &host) const
{
    return call(net_status, host);
}

fc::variant Peer::connections(const std::string &host) const
{
    return call(net_connections, host);
}

fc::variant Peer::get_account(const std::string &name) const
{
    auto arg = fc::mutable_variant_object("account_name", name);
    return call(get_account_endpoint, arg);
}

fc::variant Peer::get_block(const std::string &id_or_num) const
{
    auto arg = fc::mutable_variant_object("block_num_or_id", id_or_num);
    return call(get_block_endpoint, arg);
}

fc::variant Peer::get_key_accounts(const std::string &public_key) const
{
    auto arg = fc::mutable_variant_object( "public_key", public_key);
    return call(get_key_accounts_endpoint, arg);
}

fc::variant Peer::get_controlled_accounts(const std::string &account) const
{
    auto arg = fc::mutable_variant_object( "controlling_account", account);
    return call(get_controlled_accounts_endpoint, arg);
}

fc::variant Peer::get_transaction(const std::string &id) const
{
    auto arg = fc::mutable_variant_object( "transaction_id", id);
    return call(get_transaction_endpoint, arg);
}

fc::variant Peer::get_transactions(const std::string &account_name, const std::string &skip_seq, const std::string &num_seq) const
{
    auto arg = (skip_seq.empty())
            ? fc::mutable_variant_object( "account_name", account_name)
            : (num_seq.empty())
              ? fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq)
              : fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq)("num_seq", num_seq);
    return call(get_transactions_endpoint, arg);
}

fc::variant Peer::get_currency_balance(const std::string &account, const std::string &code, const std::string &symbol) const
{
    auto arg = fc::mutable_variant_object("json", false)
            ("account", account)
            ("code", code)
            ("symbol", symbol);
    return call(get_currency_balance_endpoint, arg);
}

fc::variant Peer::get_currency_stats(const std::string &code, const std::string &symbol) const
{
    auto arg = fc::mutable_variant_object("json", false)
            ("code", code)
            ("symbol", symbol);
    return call(get_currency_stats_endpoint, arg);
}

fc::variant Peer::push_transactions(const std::vector<chain::signed_transaction> &transactions) const
{
    return call(push_txns_endpoint, transactions);
}

fc::variant Peer::json_to_bin_function(const std::string &contract, const std::string &action, const std::string &data) const
{
    auto arg = fc::mutable_variant_object("code", contract)("action", action)("args", fc::json::from_string(data));
    return call(json_to_bin_endpoint, arg);
}

fc::variant Peer::get_keys_required(const chain::signed_transaction &transaction, const fc::variant &public_keys) const
{
    auto get_arg = fc::mutable_variant_object("transaction", transaction)("available_keys", public_keys);
    return call(get_required_keys, get_arg);
}

} // namespace client
} // namespace eosio
