/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <currency/currency.hpp> /// defines transfer struct (abi)



extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {

       if( code == N(currency) ) {
          if( action == N(transfer) ) 
             TOKEN_NAME::apply_currency_transfer( current_action< TOKEN_NAME::transfer >() );
       }
    }
}
