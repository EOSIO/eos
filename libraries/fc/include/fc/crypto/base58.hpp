#pragma once
#include <fc/string.hpp>
#include <vector>

namespace fc {
    std::string to_base58( const char* d, size_t s );
    std::string to_base58( const std::vector<char>& data );
    std::vector<char> from_base58( const std::string& base58_str );
    size_t from_base58( const std::string& base58_str, char* out_data, size_t out_data_len );
}
