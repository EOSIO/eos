#include "helloworld.h"

#include <eoslib/print.hpp>

extern "C" {

void init()  {
    eosio::print("(helloworld contract) intializing\n");
}

void apply(uint64_t code, uint64_t action) {
    eosio::print("(helloworld contract) received ", eosio::name(code), "->", eosio::name(action), "\n");

    if (code != N(helloworld))
        return;

    switch(action)
    {
    case N(sayhello):
        eosio::print("Hello World!\n");
        break;

    default:
        break;
    }
}

} // extern
