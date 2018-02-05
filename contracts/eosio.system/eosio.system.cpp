/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.system/eosio.system.hpp> 

using namespace eosiosystem;

extern "C" {

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t act ) {
       eosiosystem::contract<N(eosio.system)>::apply( code, act );
    }
}
