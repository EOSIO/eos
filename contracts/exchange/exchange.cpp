#include <math.h>
#include "exchange.hpp"

extern "C" {
   void apply( uint64_t code, uint64_t action ) {
       eosio::exchange( current_receiver() ).apply( code, action ); 
   }
}
