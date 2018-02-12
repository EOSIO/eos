/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/math.h>

namespace eosio {

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

  /**
   * Multiply two 128 bit unsigned integers and assign the value to the first parameter.
   * This wraps multeq_i128 from @ref mathcapi.
   * @brief wraps multeq_i128 from @ref mathcapi
   * @param self  Value to be multiplied. It will be replaced with the result
   * @param other Value integer to be multiplied.
   *
   * Example:
   * @code
   * uint128_t self(100);
   * uint128_t other(100);
   * multeq(self, other);
   * std::cout << self; // Output: 10000
   * @endcode
   */
  inline void multeq( uint128_t& self, const uint128_t& other ) {
     multeq_i128( &self, &other );
  }

  /**
   * Divide two 128 bit unsigned integers and assign the value to the first parameter.
   * It will throw an exception if other is zero.
   * This wraps diveq_i128 from @ref mathcapi
   * @brief wraps diveq_i128 from @ref mathcapi
   * @param self  Numerator. It will be replaced with the result
   * @param other Denominator
   *
   * Example:
   * @code
   * uint128_t self(100);
   * uint128_t other(100);
   * diveq(self, other);
   * std::cout << self; // Output: 1
   * @endcode
   */
  inline void diveq( uint128_t& self, const uint128_t& other ) {
     diveq_i128( &self, &other );
  }

  /**
   * @brief A struct that wraps uint128 integer and defines common operator overloads
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
           eosio_assert( !(value >> 64), "cast to 64 bit loss of precision" );
           return uint64_t(value);
        }

     private:
        uint128_t value = 0;
  };

   /**
    * Get the smaller of the given values
    * @brief Defined similar to std::min()
    * @param a  Value to compare
    * @param b  Value to compare
    * @return The smaller of a and b. If they are equivalent, returns a
    *
    * Example:
    * @code
    * uint128_t a(1);
    * uint128_t b(2);
    * std::cout << min(a, b); // Output: 1
    * @endcode
    */
   template<typename T>
   T min( const T& a, const T&b ) {
     return a < b ? a : b;
   }

   /**
    * Get the greater of the given values.
    * @brief Define similar to std::max()
    * @param a  Value to compare
    * @param b  Value to compare
    * @return The greater of a and b. If they are equivalent, returns a
    *
    * Example:
    * @code
    * uint128_t a(1);
    * uint128_t b(2);
    * std::cout << max(a, b); // Output: 2
    * @endcode
   */
   template<typename T>
   T max( const T& a, const T&b ) {
     return a > b ? a : b;
   }

   /// @} /// mathcppapi
}
