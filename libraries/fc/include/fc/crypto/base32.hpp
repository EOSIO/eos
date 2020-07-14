#pragma once
#include <fc/vector.hpp>
#include <fc/string.hpp>

namespace fc
{
    std::vector<char> from_base32( const fc::string& b32 );
    fc::string to_base32( const std::vector<char>& vec );
    fc::string to_base32( const char* data, size_t len );
}
