/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#ifndef WALLET_HPP
#define WALLET_HPP

#include "remote.hpp"

#include <string>

#include "eosio/chain/transaction.hpp"

namespace eosio {
namespace client {

struct Wallet : public Remote
{
    fc::variant create(const std::string &name) const;
    fc::variant open(const std::string &name) const;
    fc::variant lock(const std::string &name) const;
    fc::variant lock_all() const;
    fc::variant unlock(const std::string &name, const std::string& passwd) const;
    fc::variant import_key(const std::string &name, const std::string &passwd) const;
    fc::variant list() const;
    fc::variant list_keys() const;
    fc::variant public_keys() const;
    fc::variant sign_transaction(const chain::signed_transaction& transaction,
                                 const fc::variant& required_keys,
                                 const eosio::chain::chain_id_type& id) const;
};

} // namespace client
} // namespace eosio

#endif // WALLET_HPP
