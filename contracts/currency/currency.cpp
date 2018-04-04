/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/currency.hpp>

extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
       eosio::currency(receiver).apply( code, action ); 
    }
}
