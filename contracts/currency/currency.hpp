/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>
#include <eoslib/generic_currency.hpp>


namespace currency {
   typedef eosio::generic_currency< eosio::token<N(currency),S(4,CUR)> > contract;
}
