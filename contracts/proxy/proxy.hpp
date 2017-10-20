/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/db.hpp>
#include <eoslib/eos.hpp>

namespace proxy {
struct PACKED(Config) {
  Config(AccountName o = AccountName()) : owner(o) {}
  const uint64_t key = N(config);
  AccountName owner;
};

using Configs = Table<N(proxy), N(proxy), N(configs), Config, uint64_t>;

} /// namespace proxy