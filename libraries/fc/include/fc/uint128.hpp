#pragma once
#include <limits>
#include <stdint.h>
#include <string>

#include <fc/exception/exception.hpp>
#include <fc/crypto/city.hpp>

#ifdef _MSC_VER
  #pragma warning (push)
  #pragma warning (disable : 4244)
#endif //// _MSC_VER

namespace fc
{
  class bigint;
  /**
   *  @brief an implementation of 128 bit unsigned integer
   *
   */
  class uint128
  {


     public:
      uint128():hi(0),lo(0){}
      uint128( uint32_t l ):hi(0),lo(l){}
      uint128( int32_t l ):hi( -(l<0) ),lo(l){}
      uint128( int64_t l ):hi( -(l<0) ),lo(l){}
      uint128( uint64_t l ):hi(0),lo(l){}
      uint128( const std::string& s );
      uint128( uint64_t _h, uint64_t _l )
      :hi(_h),lo(_l){}
      uint128( const fc::bigint& bi );
      explicit uint128( unsigned __int128 i ):hi( i >> 64 ), lo(i){ }

      operator std::string()const;
      operator fc::bigint()const;

      explicit operator unsigned __int128()const {
         unsigned __int128 result(hi);
         result <<= 64;
         return result | lo;
      }

      bool     operator == ( const uint128& o )const{ return hi == o.hi && lo == o.lo;             }
      bool     operator != ( const uint128& o )const{ return hi != o.hi || lo != o.lo;             }
      bool     operator < ( const uint128& o )const { return (hi == o.hi) ? lo < o.lo : hi < o.hi; }
      bool     operator < ( const int64_t& o )const { return *this < uint128(o); }
      bool     operator !()const                    { return !(hi !=0 || lo != 0);                 }
      uint128  operator -()const                    { return ++uint128( ~hi, ~lo );                }
      uint128  operator ~()const                    { return uint128( ~hi, ~lo );                  }

      uint128& operator++()    {  hi += (++lo == 0); return *this; }
      uint128& operator--()    {  hi -= (lo-- == 0); return *this; }
      uint128  operator++(int) { auto tmp = *this; ++(*this); return tmp; }
      uint128  operator--(int) { auto tmp = *this; --(*this); return tmp; }

      uint128& operator |= ( const uint128& u ) { hi |= u.hi; lo |= u.lo; return *this; }
      uint128& operator &= ( const uint128& u ) { hi &= u.hi; lo &= u.lo; return *this; }
      uint128& operator ^= ( const uint128& u ) { hi ^= u.hi; lo ^= u.lo; return *this; }
      uint128& operator <<= ( const uint128& u );
      uint128& operator >>= ( const uint128& u );

      uint128& operator += ( const uint128& u ) { const uint64_t old = lo; lo += u.lo;  hi += u.hi + (lo < old); return *this; }
      uint128& operator -= ( const uint128& u ) { return *this += -u; }
      uint128& operator *= ( const uint128& u );
      uint128& operator /= ( const uint128& u );
      uint128& operator %= ( const uint128& u );


      friend uint128 operator + ( const uint128& l, const uint128& r )   { return uint128(l)+=r;   }
      friend uint128 operator - ( const uint128& l, const uint128& r )   { return uint128(l)-=r;   }
      friend uint128 operator * ( const uint128& l, const uint128& r )   { return uint128(l)*=r;   }
      friend uint128 operator / ( const uint128& l, const uint128& r )   { return uint128(l)/=r;   }
      friend uint128 operator % ( const uint128& l, const uint128& r )   { return uint128(l)%=r;   }
      friend uint128 operator | ( const uint128& l, const uint128& r )   { return uint128(l)=(r);  }
      friend uint128 operator & ( const uint128& l, const uint128& r )   { return uint128(l)&=r;   }
      friend uint128 operator ^ ( const uint128& l, const uint128& r )   { return uint128(l)^=r;   }
      friend uint128 operator << ( const uint128& l, const uint128& r )  { return uint128(l)<<=r;  }
      friend uint128 operator >> ( const uint128& l, const uint128& r )  { return uint128(l)>>=r;  }
      friend bool    operator >  ( const uint128& l, const uint128& r )  { return r < l;           }
      friend bool    operator >  ( const uint128& l, const int64_t& r )  { return uint128(r) < l;           }
      friend bool    operator >  ( const int64_t& l, const uint128& r )  { return r < uint128(l);           }

      friend bool    operator >=  ( const uint128& l, const uint128& r ) { return l == r || l > r; }
      friend bool    operator >=  ( const uint128& l, const int64_t& r ) { return l >= uint128(r); }
      friend bool    operator >=  ( const int64_t& l, const uint128& r ) { return uint128(l) >= r; }
      friend bool    operator <=  ( const uint128& l, const uint128& r ) { return l == r || l < r; }
      friend bool    operator <=  ( const uint128& l, const int64_t& r ) { return l <= uint128(r); }
      friend bool    operator <=  ( const int64_t& l, const uint128& r ) { return uint128(l) <= r; }

      friend std::size_t hash_value( const uint128& v ) { return city_hash_size_t((const char*)&v, sizeof(v)); }

      uint32_t to_integer()const
      {
          FC_ASSERT( hi == 0 );
          uint32_t lo32 = (uint32_t) lo;
          FC_ASSERT( lo == lo32 );
          return lo32;
      }
      uint64_t to_uint64()const
      {
          FC_ASSERT( hi == 0 );
          return lo;
      }
      uint32_t low_32_bits()const { return (uint32_t) lo; }
      uint64_t low_bits()const  { return lo; }
      uint64_t high_bits()const { return hi; }

      static uint128 max_value() {
          const uint64_t max64 = std::numeric_limits<uint64_t>::max();
          return uint128( max64, max64 );
      }

      static void full_product( const uint128& a, const uint128& b, uint128& result_hi, uint128& result_lo );

      uint8_t popcount() const;

      // fields must be public for serialization
      uint64_t hi;
      uint64_t lo;
  };
  static_assert( sizeof(uint128) == 2*sizeof(uint64_t), "validate packing assumptions" );

  typedef uint128 uint128_t;

  class variant;

  void to_variant( const uint128& var,  variant& vo, uint32_t max_depth = 1 );
  void from_variant( const variant& var,  uint128& vo, uint32_t max_depth = 1 );
//  void to_variant( const unsigned __int128& var,  variant& vo );
//  void from_variant( const variant& var,  unsigned __int128& vo );

  namespace raw
  {
    template<typename Stream>
    inline void pack( Stream& s, const uint128& u ) { s.write( (char*)&u, sizeof(u) ); }
    template<typename Stream>
    inline void unpack( Stream& s, uint128& u ) { s.read( (char*)&u, sizeof(u) ); }
  }

  size_t city_hash_size_t(const char *buf, size_t len);
} // namespace fc

namespace std
{
    template<>
    struct hash<fc::uint128>
    {
       size_t operator()( const fc::uint128& s )const
       {
           return fc::city_hash_size_t((char*)&s, sizeof(s));
       }
    };
}

FC_REFLECT( fc::uint128_t, (hi)(lo) )

#ifdef _MSC_VER
  #pragma warning (pop)
#endif ///_MSC_VER
