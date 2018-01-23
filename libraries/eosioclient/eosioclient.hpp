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

    void sign_transaction(eosio::chain::signed_transaction &trx);

    void set_server_host(const std::string& host);
    void set_server_port(uint32_t port);
    void set_wallet_host(const std::string& host);
    void set_wallet_port(uint32_t port);

    std::string host = "localhost"; /// @todo make private
    uint32_t port = 8888; /// @todo make private
    std::string wallet_host = "localhost"; /// @todo make private
    uint32_t wallet_port = 8888; /// @todo make private

private:
    fc::variant call_wallet(const std::string& path, const fc::variant& postdata = fc::variant()) const;
    fc::variant call_server(const std::string& path, const fc::variant& postdata = fc::variant()) const;

    template<typename T>
    fc::variant call_server( const std::string& path, const T& v ) const
    { return call_server(path, fc::variant(v)); }

    fc::variant call(const std::string &host, uint16_t port, const std::string &path, const fc::variant &postdata) const;
};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_HPP
