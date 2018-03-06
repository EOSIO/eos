#include "exchange.hpp"

extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t act ) {
          typedef eosio::generic_currency< eosio::token<N(eosio),S(4,EOS)> > eos;
          typedef eosio::generic_currency< eosio::token<N(currency),S(4,CUR)> >     cur;

          exchange<N(exchange), S(4,EXC), eos, cur>::apply( code, act );
    }
}
