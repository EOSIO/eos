#include <fc/crypto/hex.hpp>
#include <fc/crypto/hmac.hpp>
#include <fc/fwd_impl.hpp>
#include <openssl/sha.h>
#include <string.h>
#include <cmath>
#include <fc/crypto/sha256.hpp>
#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>
#include "_digest_common.hpp"

namespace fc {

    sha256::sha256() { memset( _hash, 0, sizeof(_hash) ); }
    sha256::sha256( const char *data, size_t size ) { 
       if (size != sizeof(_hash))	 
	  FC_THROW_EXCEPTION( exception, "sha256: size mismatch" );
       memcpy(_hash, data, size );
    }
    sha256::sha256( const string& hex_str ) {
      fc::from_hex( hex_str, (char*)_hash, sizeof(_hash) );  
    }

    string sha256::str()const {
      return fc::to_hex( (char*)_hash, sizeof(_hash) );
    }
    sha256::operator string()const { return  str(); }

    char* sha256::data()const { return (char*)&_hash[0]; }


    struct sha256::encoder::impl {
       SHA256_CTX ctx;
    };

    sha256::encoder::~encoder() {}
    sha256::encoder::encoder() {
      reset();
    }

    sha256 sha256::hash( const char* d, uint32_t dlen ) {
      encoder e;
      e.write(d,dlen);
      return e.result();
    }

    sha256 sha256::hash( const string& s ) {
      return hash( s.c_str(), s.size() );
    }

    sha256 sha256::hash( const sha256& s )
    {
        return hash( s.data(), sizeof( s._hash ) );
    }

    void sha256::encoder::write( const char* d, uint32_t dlen ) {
      SHA256_Update( &my->ctx, d, dlen); 
    }
    sha256 sha256::encoder::result() {
      sha256 h;
      SHA256_Final((uint8_t*)h.data(), &my->ctx );
      return h;
    }
    void sha256::encoder::reset() {
      SHA256_Init( &my->ctx);  
    }

    sha256 operator << ( const sha256& h1, uint32_t i ) {
      sha256 result;
      fc::detail::shift_l( h1.data(), result.data(), result.data_size(), i );
      return result;
    }
    sha256 operator >> ( const sha256& h1, uint32_t i ) {
      sha256 result;
      fc::detail::shift_r( h1.data(), result.data(), result.data_size(), i );
      return result;
    }
    sha256 operator ^ ( const sha256& h1, const sha256& h2 ) {
      sha256 result;
      result._hash[0] = h1._hash[0] ^ h2._hash[0];
      result._hash[1] = h1._hash[1] ^ h2._hash[1];
      result._hash[2] = h1._hash[2] ^ h2._hash[2];
      result._hash[3] = h1._hash[3] ^ h2._hash[3];
      return result;
    }
    bool operator >= ( const sha256& h1, const sha256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) >= 0;
    }
    bool operator > ( const sha256& h1, const sha256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) > 0;
    }
    bool operator < ( const sha256& h1, const sha256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) < 0;
    }
    bool operator != ( const sha256& h1, const sha256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) != 0;
    }
    bool operator == ( const sha256& h1, const sha256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) == 0;
    }

   uint32_t sha256::approx_log_32()const
   {
      uint16_t lzbits = clz();
      if( lzbits >= 0x100 )
         return 0;
      uint8_t nzbits = 0xFF-lzbits;
      size_t offset = (size_t) (lzbits >> 3);
      uint8_t* my_bytes = (uint8_t*) data();
      size_t n = data_size();
      uint32_t y = (uint32_t(               my_bytes[offset  ]    ) << 0x18)
                 | (uint32_t(offset+1 < n ? my_bytes[offset+1] : 0) << 0x10)
                 | (uint32_t(offset+2 < n ? my_bytes[offset+2] : 0) << 0x08)
                 | (uint32_t(offset+3 < n ? my_bytes[offset+3] : 0)        )
                 ;
      //
      // lzbits&7 == 7 : 00000001 iff nzbits&7 == 0
      // lzbits&7 == 6 : 0000001x iff nzbits&7 == 1
      // lzbits&7 == 5 : 000001xx iff nzbits&7 == 2
      //
      y >>= (nzbits & 7);
      y ^= 1 << 0x18;
      y |= uint32_t( nzbits ) << 0x18;
      return y;
   }

   void sha256::set_to_inverse_approx_log_32( uint32_t x )
   {
      uint8_t nzbits = uint8_t( x >> 0x18 );
      _hash[0] = 0;
      _hash[1] = 0;
      _hash[2] = 0;
      _hash[3] = 0;
      if( nzbits == 0 )
         return;
      uint8_t x0 = uint8_t((x        ) & 0xFF);
      uint8_t x1 = uint8_t((x >> 0x08) & 0xFF);
      uint8_t x2 = uint8_t((x >> 0x10) & 0xFF);
      uint8_t* my_bytes = (uint8_t*) data();
      my_bytes[0x1F] = x0;
      my_bytes[0x1E] = x1;
      my_bytes[0x1D] = x2;
      my_bytes[0x1C] = 1;

      if( nzbits <= 0x18 )
      {
         (*this) = (*this) >> (0x18 - nzbits);
      }
      else
         (*this) = (*this) << (nzbits - 0x18);
      return;
   }

   double sha256::inverse_approx_log_32_double( uint32_t x )
   {
      uint8_t nzbits = uint8_t( x >> 0x18 );
      if( nzbits == 0 )
         return 0.0;
      uint32_t b = 1 << 0x18;
      uint32_t y = (x & (b-1)) | b;
      return std::ldexp( y, int( nzbits ) - 0x18 );
   }

   uint16_t sha256::clz()const
   {
      const uint8_t* my_bytes = (uint8_t*) data();
      size_t size = data_size();
      size_t lzbits = 0;
      static const uint8_t char2lzbits[] = {
     // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31
        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
       };

      size_t i = 0;

      while( true )
      {
         uint8_t c = my_bytes[i];
         lzbits += char2lzbits[c];
         if( c != 0 )
            break;
         ++i;
         if( i >= size )
            return 0x100;
      }

      return lzbits;
   }

  void to_variant( const sha256& bi, variant& v, uint32_t max_depth )
  {
     v = std::vector<char>( (const char*)&bi, ((const char*)&bi) + sizeof(bi) );
  }
  void from_variant( const variant& v, sha256& bi, uint32_t max_depth )
  {
    std::vector<char> ve = v.as< std::vector<char> >( max_depth );
    if( ve.size() )
    {
        memcpy(&bi, ve.data(), fc::min<size_t>(ve.size(),sizeof(bi)) );
    }
    else
        memset( &bi, char(0), sizeof(bi) );
  }

  uint64_t hash64(const char* buf, size_t len)
  {
    sha256 sha_value = sha256::hash(buf,len);
    return sha_value._hash[0];
  }

    template<>
    unsigned int hmac<sha256>::internal_block_size() const { return 64; }
} //end namespace fc
