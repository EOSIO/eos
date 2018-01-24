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
    return m_remote.host();
}

void Wallet::set_host(const std::string &host)
{
    m_remote.set_host(host);
}

uint32_t Wallet::port() const
{
    return m_remote.port();
}

void Wallet::set_port(uint32_t port)
{
    m_remote.set_port(port);
}

fc::variant Wallet::unlock(const std::string &name, const std::string &passwd) const
{
    fc::variants vs = {fc::variant(name), fc::variant(passwd)};
    return m_remote.call(wallet_unlock, vs);
}

fc::variant Wallet::import_key(const std::string &name, const std::string &passwd) const
{
    fc::variants vs = {fc::variant(name), fc::variant(passwd)};
    return m_remote.call(wallet_import_key, vs);
}

fc::variant Wallet::list() const
{
    return m_remote.call(wallet_list);
}

fc::variant Wallet::list_keys() const
{
    return m_remote.call(wallet_list_keys);
}

fc::variant Wallet::create(const std::string& name) const
{
    return m_remote.call(wallet_create, name);
}

fc::variant Wallet::open(const std::string &name) const
{
    return m_remote.call(wallet_open, name);
}

fc::variant Wallet::lock(const std::string &name) const
{
    return m_remote.call(wallet_lock, name);
}

fc::variant Wallet::lock_all() const
{
    return m_remote.call(wallet_lock_all);
}

fc::variant Wallet::public_keys() const
{
    return m_remote.call(wallet_public_keys);
}

fc::variant Wallet::sign_transaction(const chain::signed_transaction &transaction,
                                     const fc::variant &required_keys,
                                     const chain::chain_id_type &id) const
{
    fc::variants sign_args = {fc::variant(transaction), required_keys["required_keys"], fc::variant(id)};
    return m_remote.call(wallet_sign_trx, sign_args);
}

} // namespace client
} // namespace eosio

