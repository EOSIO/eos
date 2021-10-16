#pragma once
#include <vector>
#include <string>

namespace eosio { namespace chain {

std::vector<uint8_t> wast_to_wasm( const std::string& wast );

} } /// eosio::chain
