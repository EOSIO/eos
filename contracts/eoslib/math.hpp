#pragma once
#include <eoslib/math.h>

namespace eos {

  /**
   *  @defgroup mathapi Math API
   *  @brief Defines common math functions 
   *  @ingroup contractdev
   */

  /**
   *  @defgroup mathcppapi Math C++ API
   *  @brief Defines common math functions and helper types
   *  @ingroup mathapi
   *
   *  @{
   */

  /** @brief wraps multeq_i128 from @ref mathcapi */
  inline void multeq( uint128_t& self, const uint128_t& other ) {
     multeq_i128( &self, &other );
  }

  /** @brief wraps diveq_i128 from @ref mathcapi */
  inline void diveq( uint128_t& self, const uint128_t& other ) {
     diveq_i128( &self, &other );
  }

  /**
   * @brief a struct that wraps uint128 integer and defines common operator overloads
   */
  struct uint128 {
     public:
        uint128( uint128_t i = 0 ):value(i){}
        uint128( uint64_t i ):value(i){}
        uint128( uint32_t i ):value(i){}

        friend uint128 operator * ( uint128 a, const uint128& b ) {
            return a *= b;
        }

        friend uint128 operator / ( uint128 a, const uint128& b ) {
            return a /= b;
        }
        friend bool operator <= ( const uint128& a, const uint128& b ) {
            return a.value <= b.value;
        }
        friend bool operator >= ( const uint128& a, const uint128& b ) {
            return a.value >= b.value;
        }

        uint128& operator *= ( const uint128_t& other ) { 
           multeq( value, other );
           return *this;
        }
        uint128& operator *= ( const uint128& other ) { 
           multeq( value, other.value );
           return *this;
        }

        uint128& operator /= ( const uint128_t& other ) { 
           diveq( value, other );
           return *this;
        }
        uint128& operator /= ( const uint128& other ) { 
           diveq( value, other.value );
           return *this;
        }

        explicit operator uint64_t()const {
           assert( !(value >> 64), "cast to 64 bit loss of precision" );
           return uint64_t(value);
        }

     private:
        uint128_t value = 0;
  };

   /**
    * Define similar to std::min()
    */
   template<typename T>
   T min( const T& a, const T&b ) {
     return a < b ? a : b;
   }
   /**
    * Define similar to std::max()
    */
   template<typename T>
   T max( const T& a, const T&b ) {
     return a > b ? a : b;
   }

   /// @} /// mathcppapi
}
