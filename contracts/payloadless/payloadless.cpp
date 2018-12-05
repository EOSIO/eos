/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>

using namespace eosio;

CONTRACT payloadless : public contract {
public:
   using contract::contract;

   ACTION doit() {
      print("Im a payloadless action");
   }
};

EOSIO_DISPATCH( payloadless, (doit) )
