/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */
#include <enumivolib/print.hpp> /// defines transfer struct (abi)

extern "C" {

    /// The apply method just prints forever
    void apply( uint64_t, uint64_t, uint64_t ) {
       int idx = 0;
       while(true) {
          enumivo::print(idx++);
       }
    }
}
