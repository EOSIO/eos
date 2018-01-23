#ifndef EOSIOCLIENT_HPP
#define EOSIOCLIENT_HPP

#include "fc/variant.hpp"
#include "eosio/chain/transaction.hpp"

namespace eosio {
namespace client {

class Eosioclient
{
public:
    fc::variant get_info() const;
    fc::variant get_code(const std::string &account_name) const;
    fc::variant get_table(const std::string &scope, const std::string &code, const std::string &table) const;
    fc::variant push_transaction(const chain::signed_transaction& transaction) const;
    fc::variant push_transaction(const fc::variant& transaction) const;
    fc::variant connect(const std::string& host) const;
    fc::variant disconnect(const std::string& host) const;
    fc::variant status(const std::string& host) const;
    fc::variant connections(const std::string& host) const;
    fc::variant unlock_wallet(const fc::variant& vs) const;
    fc::variant import_key_wallet(const fc::variant& vs) const;
    fc::variant list_wallet() const;
    fc::variant list_keys_wallet() const;
    fc::variant create_wallet(const std::string &name) const;
    fc::variant open_wallet(const std::string &name) const;
    fc::variant lock_wallet(const std::string &name) const;
    fc::variant lock_all_wallet() const;
    fc::variant get_account_function(const fc::mutable_variant_object& variant) const;
    fc::variant get_block_function(const fc::mutable_variant_object& variant) const;
    fc::variant get_key_accounts_function(const fc::mutable_variant_object& variant) const;
    fc::variant get_controller_accounts_function(const fc::mutable_variant_object& variant) const;
    fc::variant get_transaction_function(const fc::mutable_variant_object& variant) const;
    fc::variant push_transactions(const std::vector<chain::signed_transaction>& transactions) const;
    fc::variant json_to_bin_function(const fc::mutable_variant_object& variant) const;

    void sign_transaction(eosio::chain::signed_transaction &trx);

    std::string get_server_host() const;
    void set_server_host(const std::string& host);
    uint32_t get_server_port() const;
    void set_server_port(uint32_t port);
    std::string get_wallet_host() const;
    void set_wallet_host(const std::string& host);
    uint32_t get_wallet_port() const;
    void set_wallet_port(uint32_t port);

private:
    fc::variant call_wallet(const std::string& path, const fc::variant& postdata = fc::variant()) const;
    fc::variant call_server(const std::string& path, const fc::variant& postdata = fc::variant()) const;

    template<typename T>
    fc::variant call_server( const std::string& path, const T& v ) const
    { return call_server(path, fc::variant(v)); }

    fc::variant call(const std::string &m_server_host, uint16_t m_server_port, const std::string &path, const fc::variant &postdata) const;

    std::string m_server_host = "localhost";
    uint32_t m_server_port = 8888;
    std::string m_wallet_host = "localhost";
    uint32_t m_wallet_port = 8888;
};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_HPP
