/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eoslib/eos.hpp>
#include <eoslib/token.hpp>
#include <eoslib/db.hpp>
#include <eoslib/generic_currency.hpp>


namespace currency {
   typedef generic_currency<N(currency),
                            S(CUR), 4>  contract;
}
