#include "identity.hpp"

extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       identity::contract< N(identity) >::apply( code, action );
    }
}

