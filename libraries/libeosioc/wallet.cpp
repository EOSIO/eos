#include <eosio/libeosioc/wallet.hpp>

#include <eosio/libeosioc/endpoints.hpp>

namespace eosio {
namespace client {

fc::variant wallet::unlock(const std::string &name, const std::string &passwd) const
{
    fc::variants vs = {fc::variant(name), fc::variant(passwd)};
    return call(wallet_unlock_endpoint, vs);
}

fc::variant wallet::import_key(const std::string &name, const std::string &passwd) const
{
    fc::variants vs = {fc::variant(name), fc::variant(passwd)};
    return call(wallet_import_key_endpoint, vs);
}

fc::variant wallet::list() const
{
    return call(wallet_list_endpoint);
}

fc::variant wallet::list_keys() const
{
    return call(wallet_list_keys_endpoint);
}

fc::variant wallet::create(const std::string& name) const
{
    return call(wallet_create_endpoint, name);
}

fc::variant wallet::open(const std::string &name) const
{
    return call(wallet_open_endpoint, name);
}

fc::variant wallet::lock(const std::string &name) const
{
    return call(wallet_lock_endpoint, name);
}

fc::variant wallet::lock_all() const
{
    return call(wallet_lock_all_endpoint);
}

fc::variant wallet::public_keys() const
{
    return call(wallet_public_keys_endpoint);
}

fc::variant wallet::sign_transaction(const chain::signed_transaction &transaction,
                                     const fc::variant &required_keys,
                                     const chain::chain_id_type &id) const
{
    fc::variants sign_args = {fc::variant(transaction), required_keys["required_keys"], fc::variant(id)};
    return call(wallet_sign_trx_endpoint, sign_args);
}

} // namespace client
} // namespace eosio

