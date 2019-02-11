#pragma once
#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <string>

#include <eosiolib/token.h>

using namespace std;

namespace eosio {
    void token_create( uint64_t issuer, asset maximum_supply) {
        ::token_create( issuer, (void*)&maximum_supply, sizeof(asset));
    }
    
    void token_issue( uint64_t to, asset quantity, string memo ) {
        ::token_issue( to, &quantity, sizeof(asset), memo.c_str(), memo.size() );
    }

    void token_transfer( uint64_t from, uint64_t to, asset quantity, string memo) {
        ::token_transfer( from, to, &quantity, sizeof(asset), memo.c_str(), memo.size());
    }

}

