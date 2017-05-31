#include <fc/real128.hpp>
#include <fc/crypto/bigint.hpp>
#include <fc/exception/exception.hpp>
#include <sstream>
#include <stdint.h>

namespace fc
{
   uint64_t real128::to_uint64()const
   { 
      return (fixed/ FC_REAL128_PRECISION).to_uint64(); 
   }

   real128::real128( uint64_t integer )
   {
      fixed = uint128(integer) * FC_REAL128_PRECISION;
   }
   real128& real128::operator += ( const real128& o )
   {
      fixed += o.fixed;
      return *this;
   }
   real128& real128::operator -= ( const real128& o )
   {
      fixed -= o.fixed;
      return *this;
   }

   real128& real128::operator /= ( const real128& o )
   { try {
      FC_ASSERT( o.fixed > uint128(0), "Divide by Zero" );
       
      fc::bigint self(fixed);
      fc::bigint other(o.fixed);
      self *= FC_REAL128_PRECISION;
      self /= other;
      fixed = self;

      return *this;
   } FC_CAPTURE_AND_RETHROW( (*this)(o) ) }

   real128& real128::operator *= ( const real128& o )
   { try {
      fc::bigint self(fixed);
      fc::bigint other(o.fixed);
      self *= other;
      self /= FC_REAL128_PRECISION;
      fixed = self;
      return *this;
   } FC_CAPTURE_AND_RETHROW( (*this)(o) ) }


   real128::real128( const std::string& ratio_str )
   {
     const char* c = ratio_str.c_str();
     int digit = *c - '0';
     if (digit >= 0 && digit <= 9)
     {
       uint64_t int_part = digit;
       ++c;
       digit = *c - '0';
       while (digit >= 0 && digit <= 9)
       {
         int_part = int_part * 10 + digit;
         ++c;
         digit = *c - '0';
       }
       *this = real128(int_part);
     }
     else
     {
       // if the string doesn't look like "123.45" or ".45", this code isn't designed to parse it correctly
       // in particular, we don't try to handle leading whitespace or '+'/'-' indicators at the beginning
       assert(*c == '.');
       fixed = fc::uint128();
     }


     if (*c == '.')
     {
       c++;
       digit = *c - '0';
       if (digit >= 0 && digit <= 9)
       {
         int64_t frac_part = digit;
         int64_t frac_magnitude = 10;
         ++c;
         digit = *c - '0';
         while (digit >= 0 && digit <= 9)
         {
           frac_part = frac_part * 10 + digit;
           frac_magnitude *= 10;
           ++c;
           digit = *c - '0';
         }
         *this += real128( frac_part ) / real128( frac_magnitude );
       }
     }
   }
   real128::operator std::string()const
   {
      std::stringstream ss;
      ss << std::string(fixed / FC_REAL128_PRECISION);
      ss << '.';
      auto frac = (fixed % FC_REAL128_PRECISION) + FC_REAL128_PRECISION;
      ss << std::string( frac ).substr(1);

      auto number = ss.str();
      while(  number.back() == '0' ) number.pop_back();

      return number;
   }
   
   real128 real128::from_fixed( const uint128& fixed )
   {
       real128 result;
       result.fixed = fixed;
       return result;
   }

   void to_variant( const real128& var,  variant& vo )
   {
      vo = std::string(var);
   }
   void from_variant( const variant& var,  real128& vo )
   {
     vo = real128(var.as_string());
   }

} // namespace fc
