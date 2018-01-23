#ifndef WALLET_HPP
#define WALLET_HPP

#include <string>

#include "fc/variant.hpp"
#include "remote.hpp"

namespace eosio {
namespace client {

class Wallet
{
public:
    std::string get_wallet_host() const;
    void set_wallet_host(const std::string& host);
    uint32_t get_wallet_port() const;
    void set_wallet_port(uint32_t port);

    fc::variant unlock_wallet(const fc::variant &vs) const;
    fc::variant import_key_wallet(const fc::variant &vs) const;
    fc::variant list_wallet() const;
    fc::variant list_keys_wallet() const;
    fc::variant create_wallet(const std::string &name) const;
    fc::variant open_wallet(const std::string &name) const;
    fc::variant lock_wallet(const std::string &name) const;
    fc::variant lock_all_wallet() const;
    fc::variant public_keys() const;
    fc::variant sign_transaction(const fc::variants& args) const;

private:
    std::string m_host = "localhost";
    uint32_t m_port = 8888;
    Remote m_remote;
};

} // namespace client
} // namespace eosio

#endif // WALLET_HPP
