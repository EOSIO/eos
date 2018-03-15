/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "dice.hpp"

extern "C" {
   void apply( uint64_t code, uint64_t action ) {
      dice<N(dice)>::apply( code, action );
   }
}

