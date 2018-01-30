/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eos.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/db.hpp>
#include <eosiolib/generic_currency.hpp>


namespace currency {
   typedef eosio::generic_currency< eosio::token<N(currency),S(4,CUR)> > contract;
}
