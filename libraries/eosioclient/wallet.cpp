#include "wallet.hpp"

namespace {
const std::string wallet_func_base = "/v1/wallet";
const std::string wallet_create = wallet_func_base + "/create";
const std::string wallet_open = wallet_func_base + "/open";
const std::string wallet_list = wallet_func_base + "/list_wallets";
const std::string wallet_list_keys = wallet_func_base + "/list_keys";
const std::string wallet_public_keys = wallet_func_base + "/get_public_keys";
const std::string wallet_lock = wallet_func_base + "/lock";
const std::string wallet_lock_all = wallet_func_base + "/lock_all";
const std::string wallet_unlock = wallet_func_base + "/unlock";
const std::string wallet_import_key = wallet_func_base + "/import_key";
const std::string wallet_sign_trx = wallet_func_base + "/sign_transaction";
}

namespace eosio {
namespace client {

std::string Wallet::host() const
{
    return m_host;
}

void Wallet::set_host(const std::string &host)
{
    m_host = host;
}

uint32_t Wallet::port() const
{
    return m_port;
}

void Wallet::set_port(uint32_t port)
{
    m_port = port;
}

fc::variant Wallet::unlock(const fc::variant &vs) const
{
    return m_remote.call(m_host, m_port, wallet_unlock, vs);
}

fc::variant Wallet::import_key(const fc::variant &vs) const
{
    return m_remote.call(m_host, m_port, wallet_import_key, vs);
}

fc::variant Wallet::list() const
{
    return m_remote.call(m_host, m_port, wallet_list);
}

fc::variant Wallet::list_keys() const
{
    return m_remote.call(m_host, m_port, wallet_list_keys);
}

fc::variant Wallet::create(const std::string& name) const
{
    return m_remote.call(m_host, m_port, wallet_create, name);
}

fc::variant Wallet::open(const std::string &name) const
{
    return m_remote.call(m_host, m_port, wallet_open, name);
}

fc::variant Wallet::lock(const std::string &name) const
{
    return m_remote.call(m_host, m_port, wallet_lock, name);
}

fc::variant Wallet::lock_all() const
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

