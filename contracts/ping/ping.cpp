#include "ping.hpp"
#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>

class ping_contract : public eosio::contract
{
public:
  using eosio::contract::contract;

  // @abi action
  void ping(account_name receiver);
};

void ping_contract::ping(account_name receiver)
{
    require_auth(receiver);
    eosio::print("Pong");
}

EOSIO_ABI(ping_contract, (ping))
