#include <fc/crypto/hex.hpp>
#include <fc/crypto/hmac.hpp>
#include <fc/fwd_impl.hpp>
#include <openssl/sha.h>
#include <string.h>
#include <fc/crypto/sha512.hpp>
#include <fc/variant.hpp>
#include "_digest_common.hpp"

namespace fc {

    sha512::sha512() { memset( _hash, 0, sizeof(_hash) ); }
    sha512::sha512( const string& hex_str ) {
      auto bytes_written = fc::from_hex( hex_str, (char*)_hash, sizeof(_hash) );
      if( bytes_written < sizeof(_hash) )
         memset( (char*)_hash + bytes_written, 0, (sizeof(_hash) - bytes_written) );
    }

    string sha512::str()const {
      return fc::to_hex( (char*)_hash, sizeof(_hash) );
    }
    sha512::operator string()const { return  str(); }

    char* sha512::data()const { return (char*)&_hash[0]; }


    struct sha512::encoder::impl {
       SHA512_CTX ctx;
    };

    sha512::encoder::~encoder() {}
    sha512::encoder::encoder() {
      reset();
    }

    sha512 sha512::hash( const char* d, uint32_t dlen ) {
      encoder e;
      e.write(d,dlen);
      return e.result();
    }
    sha512 sha512::hash( const string& s ) {
      return hash( s.c_str(), s.size() );
    }

    void sha512::encoder::write( const char* d, uint32_t dlen ) {
      SHA512_Update( &my->ctx, d, dlen);
    }
    sha512 sha512::encoder::result() {
      sha512 h;
      SHA512_Final((uint8_t*)h.data(), &my->ctx );
      return h;
    }
    void sha512::encoder::reset() {
      SHA512_Init( &my->ctx);
    }

    sha512 operator << ( const sha512& h1, uint32_t i ) {
      sha512 result;
      fc::detail::shift_l( h1.data(), result.data(), result.data_size(), i );
      return result;
    }
    sha512 operator ^ ( const sha512& h1, const sha512& h2 ) {
      sha512 result;
      result._hash[0] = h1._hash[0] ^ h2._hash[0];
      result._hash[1] = h1._hash[1] ^ h2._hash[1];
      result._hash[2] = h1._hash[2] ^ h2._hash[2];
      result._hash[3] = h1._hash[3] ^ h2._hash[3];
      result._hash[4] = h1._hash[4] ^ h2._hash[4];
      result._hash[5] = h1._hash[5] ^ h2._hash[5];
      result._hash[6] = h1._hash[6] ^ h2._hash[6];
      result._hash[7] = h1._hash[7] ^ h2._hash[7];
      return result;
    }
    bool operator >= ( const sha512& h1, const sha512& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) >= 0;
    }
    bool operator > ( const sha512& h1, const sha512& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) > 0;
    }
    bool operator < ( const sha512& h1, const sha512& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) < 0;
    }
    bool operator != ( const sha512& h1, const sha512& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) != 0;
    }
    bool operator == ( const sha512& h1, const sha512& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) == 0;
    }

  void to_variant( const sha512& bi, variant& v )
  {
     v = std::vector<char>( (const char*)&bi, ((const char*)&bi) + sizeof(bi) );
  }
  void from_variant( const variant& v, sha512& bi )
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
    unsigned int hmac<sha512>::internal_block_size() const { return 128; }
}
