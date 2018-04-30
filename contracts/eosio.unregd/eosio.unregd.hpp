#include <string>

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/fixed_key.hpp>

// Macro
#define TABLE(X) ::eosio::string_to_name(#X)

// Typedefs
typedef std::string ethereum_account;

// Namespaces
using eosio::const_mem_fun;
using eosio::indexed_by;
using eosio::key256;
using std::string;

namespace eosio {

class unregd : public contract {
 public:
  unregd(account_name contract_account)
      : eosio::contract(contract_account), accounts(contract_account, contract_account) {}

  // Actions
  void add(const ethereum_account& ethereum_account, const asset& balance);

 private:
  static const uint64_t EOS_PRECISION = 4;
  static const asset_symbol EOS_SYMBOL = S(EOS_PRECISION, EOS);

  //@abi table accounts i64
  struct account {
    uint64_t id;
    ethereum_account ethereum_account;
    asset balance;

    uint64_t primary_key() const { return id; }

    EOSLIB_SERIALIZE(account, (id)(ethereum_account)(balance))
  };

  typedef eosio::multi_index<TABLE(accounts), account> accounts_index;

  accounts_index::const_iterator find_ethereum_account(const ethereum_account& ethereum_account) const;

  accounts_index accounts;
};

}  // namespace eosio