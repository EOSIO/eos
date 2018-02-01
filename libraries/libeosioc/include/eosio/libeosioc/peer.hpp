/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include "remote.hpp"

#include <string>
#include <eosio/chain/transaction.hpp>

namespace eosio {
namespace client {

struct peer : public remote
{
    fc::variant connect_to(const std::string& host) const;
    fc::variant disconnect_from(const std::string& host) const;
    fc::variant status(const std::string& host) const;
    fc::variant connections(const std::string& host) const;
    fc::variant get_info() const;
    fc::variant get_code(const std::string& account_name) const;
    fc::variant get_table(const std::string& scope,
                          const std::string& code,
                          const std::string& table) const;
    fc::variant get_account(const std::string& name) const;
    fc::variant get_block(const std::string& id_or_num) const;
    fc::variant get_key_accounts(const std::string& public_key) const;
    fc::variant get_controlled_accounts(const std::string& account) const;
    fc::variant get_keys_required(const chain::signed_transaction& transaction,
                                  const fc::variant& public_keys) const;
    fc::variant get_transaction(const std::string& id) const;
    fc::variant get_transactions(const std::string& account_name,
                                 const std::string& skip_seq,
                                 const std::string& num_seq) const;
    fc::variant get_currency_balance(const std::string& account,
                                     const std::string& code,
                                     const std::string& symbol) const;
    fc::variant get_currency_stats(const std::string& code,
                                   const std::string& symbol) const;
    fc::variant push_transaction(const chain::signed_transaction& transaction) const;
    fc::variant push_transactions(const std::vector<chain::signed_transaction>& transactions) const;
    fc::variant push_JSON_transaction(const fc::variant& transaction) const;
    fc::variant json_to_bin(const std::string& contract,
                            const std::string& action,
                            const std::string& data) const;
};

} // namespace client
} // namespace eosio
