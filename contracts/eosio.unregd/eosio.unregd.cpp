#include <algorithm>

#include "eosio.unregd.hpp"

using eosio::unregd;

EOSIO_ABI(eosio::unregd, (add))

/**
 * Add a mapping betwen an ethereum_account and an initial EOS token balance.
 */
void unregd::add(const ethereum_account& ethereum_account, const asset& balance) {
  require_auth(_self);

  auto symbol = balance.symbol;
  eosio_assert(symbol.is_valid() && symbol == EOS_SYMBOL, "balance must be EOS token");

  // TODO: What to do with existing mapping? Replace? Error?
  auto itr = find_ethereum_account(ethereum_account);
  if (itr == accounts.end()) {
    accounts.emplace(_self, [&](auto& account) {
      account.id = accounts.available_primary_key();
      account.ethereum_account = ethereum_account;
      account.balance = balance;
    });
  } else {
    accounts.modify(itr, _self, [&](auto& account) {
      account.ethereum_account = ethereum_account;
      account.balance = balance;
    });
  }
}

/**
 * For now, in `multi_index`, there is no possibility to create a secondary index
 * based on a `std::string` or a fixed sized array (unless a key256). As such,
 * for now, let's do a poor man search to find a matching account.
 *
 * TODO: Use a key256 type instead. The `ethereum_account` (40 hex characters,
 * 80 bytes) would then be the 80 first bytes of the key and the rest would be
 * padded with zeros creating a key256 from an ethereum account.
 */
unregd::accounts_index::const_iterator unregd::find_ethereum_account(const ethereum_account& ethereum_account) const {
  return std::find_if(accounts.cbegin(), accounts.cend(),
                      [&](auto& account) { return account.ethereum_account == ethereum_account; });
}
