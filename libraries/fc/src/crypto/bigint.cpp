#include <fc/crypto/bigint.hpp>
#include <fc/utility.hpp>
#include <openssl/bn.h>
#include <fc/variant.hpp>
#include <fc/crypto/base64.hpp>

#include <fc/exception/exception.hpp>
#include "../byteswap.hpp"

namespace fc {
      bigint::bigint( const char* bige, uint32_t l ) {
        n = BN_bin2bn( (const unsigned char*)bige, l, NULL );
        FC_ASSERT( n != nullptr );
      }
      bigint::bigint( const std::vector<char>& bige ) {
        n = BN_bin2bn( (const unsigned char*)bige.data(), bige.size(), NULL );
        FC_ASSERT( n != nullptr );
      }
      bigint::bigint( BIGNUM* in )
      {
        n = BN_dup(in);
      }
      bigint::bigint( )
      :n(BN_new()) 
      { }

      BIGNUM* bigint::dup()const
      {
        return BN_dup( n );
      }

      bigint::bigint(uint64_t value)
      {
        uint64_t big_endian_value = bswap_64(value);
        n = BN_bin2bn((const unsigned char*)&big_endian_value, sizeof(big_endian_value), NULL);
      }

      bigint::bigint( const bigint& c ) {
        n = BN_dup( c.n );
      }

      bigint::bigint( bigint&& b ) {
        n = b.n;
        b.n = 0;
      }

      bigint::~bigint() {
        if(n!=0) BN_free(n);
      }

      bool bigint::is_negative()const { return BN_is_negative(n); }
      
      int64_t bigint::to_int64() const
      {
        FC_ASSERT(BN_num_bits(n) <= 63);
        size_t size = BN_num_bytes(n);
        uint64_t abs_value = 0;
        BN_bn2bin(n, (unsigned char*)&abs_value + (sizeof(uint64_t) - size));
        return BN_is_negative(n) ? -(int64_t)bswap_64(abs_value) : bswap_64(abs_value);
      }

      int64_t bigint::log2()const { return BN_num_bits(n); }
      bool bigint::operator < ( const bigint& c )const {
        return BN_cmp( n, c.n ) < 0;
      }
      bool bigint::operator > ( const bigint& c )const {
        return BN_cmp( n, c.n ) > 0;
      }
      bool bigint::operator >= ( const bigint& c )const {
        return BN_cmp( n, c.n ) >= 0;
      }
      bool bigint::operator == ( const bigint& c )const {
        return BN_cmp( n, c.n ) == 0;
      }
      bool bigint::operator != ( const bigint& c )const {
        return BN_cmp( n, c.n ) != 0;
      }
      bigint::operator bool()const
      {
        return !BN_is_zero( n );
      }
      bigint bigint::operator++(int)
      {
        bigint tmp = *this;
        *this = *this + bigint(1);
        return tmp;
      }
      bigint& bigint::operator++()
      {
        return *this = *this + bigint(1);
      }
      bigint bigint::operator--(int)
      {
        bigint tmp = *this;
        *this = *this - bigint(1);
        return tmp;
      }
      bigint& bigint::operator--()
      {
        return *this = *this - bigint(1);
      }

      bigint bigint::operator + ( const bigint& a )const {
        bigint tmp(*this);
        BN_add( tmp.n, n, a.n );
        return tmp;
      }
      bigint& bigint::operator += ( const bigint& a ){
        bigint tmp(*this);
        BN_add( tmp.n, n, a.n );
        std::swap(*this,tmp);
        return *this;
      }
      bigint& bigint::operator -= ( const bigint& a ){
        bigint tmp(*this);
        BN_sub( tmp.n, n, a.n );
        std::swap(*this,tmp);
        return *this;
      }


      bigint bigint::operator * ( const bigint& a )const {
        BN_CTX* ctx = BN_CTX_new();
        bigint tmp(*this);
        BN_mul( tmp.n, n, a.n, ctx );
        BN_CTX_free(ctx);
        return tmp;
      }
      bigint bigint::operator / ( const bigint& a ) const {
        BN_CTX* ctx = BN_CTX_new();
        bigint tmp;//(*this);
        BN_div( tmp.n, NULL, n, a.n, ctx );
        BN_CTX_free(ctx);
        return tmp;
      }
      bigint bigint::operator % ( const bigint& a ) const {
        BN_CTX* ctx = BN_CTX_new();
        bigint tmp;//(*this);
        BN_mod( tmp.n, n, a.n, ctx );
        BN_CTX_free(ctx);
        return tmp;
      }

      bigint bigint::operator /= ( const bigint& a ) {
        BN_CTX* ctx = BN_CTX_new();
        bigint tmp;//*this);
        BN_div( tmp.n, NULL, n, a.n, ctx );
        fc_swap( tmp.n, n );
        BN_CTX_free(ctx);
        return tmp;
      }
      bigint bigint::operator *= ( const bigint& a ) {
        auto tmp = *this * a;
        *this = std::move(tmp);
        return *this;
      }
      bigint& bigint::operator >>= ( uint32_t i )
      {
         bigint tmp;
         BN_rshift( tmp.n, n, i );
         std::swap(*this,tmp);
         return *this;
      }

      bigint& bigint::operator <<= ( uint32_t i )
      {
         bigint tmp;
         FC_ASSERT( tmp.n != nullptr );
         FC_ASSERT( n != nullptr );
         BN_lshift( tmp.n, n, i );
         std::swap(*this,tmp);
         return *this;
      }

      bigint bigint::operator - ( const bigint& a )const {
        bigint tmp;
        BN_sub( tmp.n, n, a.n );
        return tmp;
      }
      bigint bigint::exp( const bigint& a )const 
      {
        BN_CTX* ctx = BN_CTX_new();
        bigint tmp;
        BN_exp( tmp.n, n, a.n, ctx );
        BN_CTX_free(ctx);
        return tmp;
      }


      bigint& bigint::operator = ( bigint&& a ) {
        fc_swap( a.n, n );
        return *this;
      }
      bigint& bigint::operator = ( const bigint& a ) {
        if( &a == this ) 
          return *this;
        BN_copy( n, a.n );
        return *this;
      }
      bigint::operator fc::string()const {
        return BN_bn2dec(n);
      }

      bigint::operator std::vector<char>()const {
        std::vector<char> to(BN_num_bytes(n)); 
        BN_bn2bin(n,(unsigned char*)to.data());
        return to;
      }

  /** encodes the big int as base64 string, or a number */
  void to_variant( const bigint& bi, variant& v, uint32_t )
  {
    std::vector<char> ve = bi;
    v = fc::variant(base64_encode((unsigned char*)ve.data(),ve.size()));
  }

  /** decodes the big int as base64 string, or a number */
  void from_variant( const variant& v, bigint& bi, uint32_t )
  {
    if( v.is_numeric() ) bi = bigint( static_cast<unsigned long>(v.as_uint64( )) );
    else
    {
        std::string b64 = v.as_string();
        std::string bin = base64_decode(b64);
        bi = bigint(bin.c_str(), bin.size() );
    }
  }
  
} // namespace fc 
