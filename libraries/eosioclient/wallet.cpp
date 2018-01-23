#include "wallet.hpp"

#include "functions.hpp"

namespace eosio {
namespace client {

std::string Wallet::get_wallet_host() const
{
    return m_host;
}

void Wallet::set_wallet_host(const std::string &host)
{
    m_host = host;
}

uint32_t Wallet::get_wallet_port() const
{
    return m_port;
}

void Wallet::set_wallet_port(uint32_t port)
{
    m_port = port;
}

fc::variant Wallet::unlock_wallet(const fc::variant &vs) const
{
    return m_remote.call(m_host, m_port, wallet_unlock, vs);
}

fc::variant Wallet::import_key_wallet(const fc::variant &vs) const
{
    return m_remote.call(m_host, m_port, wallet_import_key, vs);
}

fc::variant Wallet::list_wallet() const
{
    return m_remote.call(m_host, m_port, wallet_list);
}

fc::variant Wallet::list_keys_wallet() const
{
    return m_remote.call(m_host, m_port, wallet_list_keys);
}

fc::variant Wallet::create_wallet(const std::string& name) const
{
    return m_remote.call(m_host, m_port, wallet_create, name);
}

fc::variant Wallet::open_wallet(const std::string &name) const
{
    return m_remote.call(m_host, m_port, wallet_open, name);
}

fc::variant Wallet::lock_wallet(const std::string &name) const
{
    return m_remote.call(m_host, m_port, wallet_lock, name);
}

fc::variant Wallet::lock_all_wallet() const
{
    return m_remote.call(m_host, m_port, wallet_lock_all);
}

fc::variant Wallet::public_keys() const
{
    return m_remote.call(m_host, m_port, wallet_public_keys);
}

fc::variant Wallet::sign_transaction(const fc::variants &args) const
{
    return m_remote.call(m_host, m_port, wallet_sign_trx, args);
}

} // namespace client
} // namespace eosio

