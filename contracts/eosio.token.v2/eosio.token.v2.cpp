/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include "eosio.token.v2.hpp"
#include <eosiolib/token.hpp>

namespace eosio {

void token::create( account_name issuer,
                    asset        maximum_supply )
{
   token_create(issuer, maximum_supply);
}


void token::issue( account_name to, asset quantity, string memo )
{
   token_issue(to, quantity, memo);
}

void token::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
   token_transfer(from, to, quantity, memo);
}

} /// namespace eosio

EOSIO_ABI( eosio::token, (create)(issue)(transfer) )

