#include <fc/crypto/hex.hpp>
#include <fc/fwd_impl.hpp>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <string.h>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/variant.hpp>
#include <vector>
#include "_digest_common.hpp"

namespace fc 
{
  
ripemd160::ripemd160() { memset( _hash, 0, sizeof(_hash) ); }
ripemd160::ripemd160( const string& hex_str ) {
  fc::from_hex( hex_str, (char*)_hash, sizeof(_hash) );  
}

string ripemd160::str()const {
  return fc::to_hex( (char*)_hash, sizeof(_hash) );
}
ripemd160::operator string()const { return  str(); }

char* ripemd160::data()const { return (char*)&_hash[0]; }


struct ripemd160::encoder::impl {
   impl()
   {
        memset( (char*)&ctx, 0, sizeof(ctx) );
   }
   RIPEMD160_CTX ctx;
};

ripemd160::encoder::~encoder() {}
ripemd160::encoder::encoder() {
  reset();
}

ripemd160 ripemd160::hash( const fc::sha512& h )
{
  return hash( (const char*)&h, sizeof(h) );
}
ripemd160 ripemd160::hash( const fc::sha256& h )
{
  return hash( (const char*)&h, sizeof(h) );
}
ripemd160 ripemd160::hash( const char* d, uint32_t dlen ) {
  encoder e;
  e.write(d,dlen);
  return e.result();
}
ripemd160 ripemd160::hash( const string& s ) {
  return hash( s.c_str(), s.size() );
}

void ripemd160::encoder::write( const char* d, uint32_t dlen ) {
  RIPEMD160_Update( &my->ctx, d, dlen); 
}
ripemd160 ripemd160::encoder::result() {
  ripemd160 h;
  RIPEMD160_Final((uint8_t*)h.data(), &my->ctx );
  return h;
}
void ripemd160::encoder::reset() {
  RIPEMD160_Init( &my->ctx);  
}

ripemd160 operator << ( const ripemd160& h1, uint32_t i ) {
  ripemd160 result;
  fc::detail::shift_l( h1.data(), result.data(), result.data_size(), i );
  return result;
}
ripemd160 operator ^ ( const ripemd160& h1, const ripemd160& h2 ) {
  ripemd160 result;
  result._hash[0] = h1._hash[0] ^ h2._hash[0];
  result._hash[1] = h1._hash[1] ^ h2._hash[1];
  result._hash[2] = h1._hash[2] ^ h2._hash[2];
  result._hash[3] = h1._hash[3] ^ h2._hash[3];
  result._hash[4] = h1._hash[4] ^ h2._hash[4];
  return result;
}
bool operator >= ( const ripemd160& h1, const ripemd160& h2 ) {
  return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) >= 0;
}
bool operator > ( const ripemd160& h1, const ripemd160& h2 ) {
  return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) > 0;
}
bool operator < ( const ripemd160& h1, const ripemd160& h2 ) {
  return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) < 0;
}
bool operator != ( const ripemd160& h1, const ripemd160& h2 ) {
  return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) != 0;
}
bool operator == ( const ripemd160& h1, const ripemd160& h2 ) {
  return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) == 0;
}
  
  void to_variant( const ripemd160& bi, variant& v, uint32_t )
  {
     v = std::vector<char>( (const char*)&bi, ((const char*)&bi) + sizeof(bi) );
  }
  void from_variant( const variant& v, ripemd160& bi, uint32_t max_depth )
  {
    std::vector<char> ve = v.as< std::vector<char> >( max_depth );
    if( ve.size() )
    {
        memcpy(&bi, ve.data(), fc::min<size_t>(ve.size(),sizeof(bi)) );
    }
    else
        memset( &bi, char(0), sizeof(bi) );
  }
  
} // fc
