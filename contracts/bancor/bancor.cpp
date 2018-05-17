/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */

#include <bancor/bancor.hpp>

namespace bancor {

extern "C" {

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t r, uint64_t c, uint64_t a ) {
       bancor::example_converter::apply( c, a );
    }
}

}
