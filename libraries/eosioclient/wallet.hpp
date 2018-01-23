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
    std::string host() const;
    void set_host(const std::string& host);
    uint32_t port() const;
    void set_port(uint32_t port);

    fc::variant create(const std::string &name) const;
    fc::variant open(const std::string &name) const;
    fc::variant lock(const std::string &name) const;
    fc::variant lock_all() const;
    fc::variant unlock(const fc::variant &vs) const;
    fc::variant import_key(const fc::variant &vs) const;
    fc::variant list() const;
    fc::variant list_keys() const;
    fc::variant public_keys() const;
    fc::variant sign_transaction(const fc::variants& args) const;

private:
    Remote m_remote;
};

} // namespace client
} // namespace eosio

#endif // WALLET_HPP
