/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.system/eosio.system.hpp> 

using namespace eosiosystem;

extern "C" {

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t receiver, uint64_t code, uint64_t act ) {
       //print( eosio::name(code), "::", eosio::name(act) );
       eosiosystem::contract<N(eosio)>::apply( receiver, code, act );
    }
}
