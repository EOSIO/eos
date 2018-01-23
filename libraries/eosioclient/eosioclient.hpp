#ifndef EOSIOCLIENT_HPP
#define EOSIOCLIENT_HPP

#include "fc/variant.hpp"
#include "eosio/chain/transaction.hpp"

namespace eosio {
namespace client {

class Eosioclient
{
public:


    void sign_transaction(eosio::chain::signed_transaction &trx);

    std::string get_server_host() const;
    void set_server_host(const std::string& host);
    uint32_t get_server_port() const;
    void set_server_port(uint32_t port);

private:
    fc::variant call_server(const std::string& path, const fc::variant& postdata = fc::variant()) const;

    template<typename T>
    fc::variant call_server( const std::string& path, const T& v ) const
    { return call_server(path, fc::variant(v)); }

    fc::variant call(const std::string &m_server_host, uint16_t m_server_port, const std::string &path, const fc::variant &postdata) const;

    std::string m_server_host = "localhost";
    uint32_t m_server_port = 8888;
};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_HPP
