/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp> /// defines transfer struct (abi)

using namespace eosio;

extern "C" {
   /// The apply method just prints forever
   void apply( uint64_t, uint64_t, uint64_t ) {
      int idx = 0;
      while( true ) {
         eosio::print( idx++ );
      }
   }
}
