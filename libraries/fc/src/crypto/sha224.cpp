#include <fc/crypto/hex.hpp>
#include <fc/crypto/hmac.hpp>
#include <fc/fwd_impl.hpp>
#include <openssl/sha.h>
#include <string.h>
#include <fc/crypto/sha224.hpp>
#include <fc/variant.hpp>
#include "_digest_common.hpp"

namespace fc {

    sha224::sha224() { memset( _hash, 0, sizeof(_hash) ); }
    sha224::sha224( const string& hex_str ) {
      auto bytes_written = fc::from_hex( hex_str, (char*)_hash, sizeof(_hash) );
      if( bytes_written < sizeof(_hash) )
         memset( (char*)_hash + bytes_written, 0, (sizeof(_hash) - bytes_written) );
    }

    string sha224::str()const {
      return fc::to_hex( (char*)_hash, sizeof(_hash) );
    }
    sha224::operator string()const { return  str(); }

    char* sha224::data()const { return (char*)&_hash[0]; }


    struct sha224::encoder::impl {
       SHA256_CTX ctx;
    };

    sha224::encoder::~encoder() {}
    sha224::encoder::encoder() {
      reset();
    }

    sha224 sha224::hash( const char* d, uint32_t dlen ) {
      encoder e;
      e.write(d,dlen);
      return e.result();
    }
    sha224 sha224::hash( const string& s ) {
      return hash( s.c_str(), s.size() );
    }

    void sha224::encoder::write( const char* d, uint32_t dlen ) {
      SHA224_Update( &my->ctx, d, dlen);
    }
    sha224 sha224::encoder::result() {
      sha224 h;
      SHA224_Final((uint8_t*)h.data(), &my->ctx );
      return h;
    }
    void sha224::encoder::reset() {
      SHA224_Init( &my->ctx);
    }

    sha224 operator << ( const sha224& h1, uint32_t i ) {
      sha224 result;
      fc::detail::shift_l( h1.data(), result.data(), result.data_size(), i );
      return result;
    }
    sha224 operator ^ ( const sha224& h1, const sha224& h2 ) {
      sha224 result;
      for( uint32_t i = 0; i < 7; ++i )
      result._hash[i] = h1._hash[i] ^ h2._hash[i];
      return result;
    }
    bool operator >= ( const sha224& h1, const sha224& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(sha224) ) >= 0;
    }
    bool operator > ( const sha224& h1, const sha224& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(sha224) ) > 0;
    }
    bool operator < ( const sha224& h1, const sha224& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(sha224) ) < 0;
    }
    bool operator != ( const sha224& h1, const sha224& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(sha224) ) != 0;
    }
    bool operator == ( const sha224& h1, const sha224& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(sha224) ) == 0;
    }

  void to_variant( const sha224& bi, variant& v )
  {
     v = std::vector<char>( (const char*)&bi, ((const char*)&bi) + sizeof(bi) );
  }
  void from_variant( const variant& v, sha224& bi )
  {
    std::vector<char> ve = v.as< std::vector<char> >();
    if( ve.size() )
    {
        memcpy(&bi, ve.data(), fc::min<size_t>(ve.size(),sizeof(bi)) );
    }
    else
        memset( &bi, char(0), sizeof(bi) );
  }

    template<>
    unsigned int hmac<sha224>::internal_block_size() const { return 64; }
}
