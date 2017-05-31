#pragma once
#include <fc/vector.hpp>
#include <fc/string.hpp>

namespace fc
{
    std::vector<char> from_base36( const fc::string& b36 );
    fc::string to_base36( const std::vector<char>& vec );
    fc::string to_base36( const char* data, size_t len );
}
