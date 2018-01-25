#include "wallet.hpp"

#include "functions.hpp"

namespace eosio {
namespace client {

fc::variant Wallet::unlock(const std::string &name, const std::string &passwd) const
{
    fc::variants vs = {fc::variant(name), fc::variant(passwd)};
    return call(wallet_unlock, vs);
}

fc::variant Wallet::import_key(const std::string &name, const std::string &passwd) const
{
    fc::variants vs = {fc::variant(name), fc::variant(passwd)};
    return call(wallet_import_key, vs);
}

fc::variant Wallet::list() const
{
    return call(wallet_list);
}

fc::variant Wallet::list_keys() const
{
    return call(wallet_list_keys);
}

fc::variant Wallet::create(const std::string& name) const
{
    return call(wallet_create, name);
}

fc::variant Wallet::open(const std::string &name) const
{
    return call(wallet_open, name);
}

fc::variant Wallet::lock(const std::string &name) const
{
    return call(wallet_lock, name);
}

fc::variant Wallet::lock_all() const
{
    return call(wallet_lock_all);
}

fc::variant Wallet::public_keys() const
{
    return call(wallet_public_keys);
}

fc::variant Wallet::sign_transaction(const chain::signed_transaction &transaction,
                                     const fc::variant &required_keys,
                                     const chain::chain_id_type &id) const
{
    fc::variants sign_args = {fc::variant(transaction), required_keys["required_keys"], fc::variant(id)};
    return call(wallet_sign_trx, sign_args);
}

} // namespace client
} // namespace eosio

