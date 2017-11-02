#pragma once
#include <eosio/blockchain/types.hpp>

namespace eosio { namespace blockchain {

vector<uint8_t> wast_to_wasm( const string& wast );
string          wasm_to_wast( const vector<uint8_t>& wasm );
string          wasm_to_wast( const uint8_t* data, uint64_t size );

} } /// eosio::blockchain
