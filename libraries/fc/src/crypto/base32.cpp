#include <fc/crypto/base32.hpp>
#include <CyoDecode.h>
#include <CyoEncode.h>
namespace fc
{
    std::vector<char> from_base32( const std::string& b32 )
    {
       auto len = cyoBase32DecodeGetLength( b32.size() );
       std::vector<char> v(len);
       len = cyoBase32Decode( v.data(), b32.c_str(), b32.size() );
       v.resize( len );
       return v;
    }

    std::string to_base32( const char* data, size_t len )
    { 
       auto s = cyoBase32EncodeGetLength(len);
       std::vector<char> b32;
       b32.resize(s);
       cyoBase32Encode( b32.data(), data, len );
       b32.resize( b32.size()-1); // strip the nullterm
       return std::string(b32.begin(),b32.end());
    }

    std::string to_base32( const std::vector<char>& vec )
    {
       return to_base32( vec.data(), vec.size() );
    }
}
