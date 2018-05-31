#pragma once
#include <fc/uint128.hpp>   

#define FC_REAL128_PRECISION (uint64_t(1000000) * uint64_t(1000000) * uint64_t(1000000))

namespace fc {
   class variant;

   /**
    * Provides fixed point math operations based on decimal fractions
    * with 18 places.
    * Delegates to fc::bigint for multiplication and division.
    */
   class real128
   {
      public:
         real128( uint64_t integer = 0);
         explicit real128( const std::string& str );
         operator std::string()const;

         friend real128 operator * ( real128 a, const real128& b ) { a *= b; return a; }
         friend real128 operator / ( real128 a, const real128& b ) { a /= b; return a; }
         friend real128 operator + ( real128 a, const real128& b ) { a += b; return a; }
         friend real128 operator - ( real128 a, const real128& b ) { a -= b; return a; }

         real128& operator += ( const real128& o );
         real128& operator -= ( const real128& o );
         real128& operator /= ( const real128& o );
         real128& operator *= ( const real128& o );
         
         static real128 from_fixed( const uint128& fixed );

         uint64_t to_uint64()const;

      private:
         uint128  fixed;
   };

   void to_variant( const real128& var,  variant& vo, uint32_t max_depth = 1 );
   void from_variant( const variant& var,  real128& vo, uint32_t max_depth = 1 );

  namespace raw  
  {
    template<typename Stream>
    inline void pack( Stream& s, const real128& value_to_pack ) { s.write( (char*)&value_to_pack, sizeof(value_to_pack) ); }
    template<typename Stream>
    inline void unpack( Stream& s, real128& value_to_unpack ) { s.read( (char*)&value_to_unpack, sizeof(value_to_unpack) ); }
  }



} // namespace fc
