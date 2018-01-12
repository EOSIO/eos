#include "helloworld.h"

#include <eoslib/print.hpp>

extern "C" {
void init()  {
    eosio::print("intializing Hello World contract\n");
}

void apply( uint64_t code, uint64_t action ) {
    eosio::print( "Hello World: ", eosio::name(code), "->", eosio::name(action), "\n" );

    if (code == N(helloworld))
        if (action == N(helloworld))
            eosio::print("Hello World!");
}
}

