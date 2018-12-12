/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

using namespace eosio;

CONTRACT hello : public contract {
public:
   using contract::contract;

   ACTION hi( name user ) {
      print( "Hello, ", user );
   }
};

EOSIO_DISPATCH( hello, (hi) )
