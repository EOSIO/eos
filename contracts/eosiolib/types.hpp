/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.h>
#include <functional>
#include <tuple>

namespace eosio {

   /**
    *  Converts a base32 symbol into its binary representation, used by string_to_name()
    * 
    *  @brief Converts a base32 symbol into its binary representation, used by string_to_name()
    *  @param c - Character to be converted
    *  @return constexpr char - Converted character
    *  @ingroup types
    */
   static constexpr  char char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      return 0;
   }


   /**
    *  Converts a base32 string to a uint64_t. This is a constexpr so that
    *  this method can be used in template arguments as well.
    * 
    *  @brief Converts a base32 string to a uint64_t.
    *  @param str - String representation of the name
    *  @return constexpr uint64_t - 64-bit unsigned integer representation of the name
    *  @ingroup types
    */
   static constexpr uint64_t string_to_name( const char* str ) {

      uint32_t len = 0;
      while( str[len] ) ++len;

      uint64_t value = 0;

      for( uint32_t i = 0; i <= 12; ++i ) {
         uint64_t c = 0;
         if( i < len && i <= 12 ) c = uint64_t(char_to_symbol( str[i] ));

         if( i < 12 ) {
            c &= 0x1f;
            c <<= 64-5*(i+1);
         }
         else {
            c &= 0x0f;
         }

         value |= c;
      }

      return value;
   }

   /**
    * Used to generate a compile time uint64_t from the base32 encoded string interpretation of X
    * 
    * @brief Used to generate a compile time uint64_t from the base32 encoded string interpretation of X
    * @param X - String representation of the name
    * @return constexpr uint64_t - 64-bit unsigned integer representation of the name
    * @ingroup types
    */
   #define N(X) ::eosio::string_to_name(#X)

   /**
    *  Wraps a uint64_t to ensure it is only passed to methods that expect a Name and
    *  that no mathematical operations occur.  It also enables specialization of print
    *  so that it is printed as a base32 string.
    *
    *  @brief wraps a uint64_t to ensure it is only passed to methods that expect a Name
    *  @ingroup types
    */
   struct name {
      /**
       * Conversion Operator to convert name to uint64_t
       * 
       * @brief Conversion Operator
       * @return uint64_t - Converted result
       */
      operator uint64_t()const { return value; }

      /**
       * Equality Operator for name
       * 
       * @brief Equality Operator for name
       * @param a - First data to be compared
       * @param b - Second data to be compared
       * @return true - if equal 
       * @return false - if unequal
       */
      friend bool operator==( const name& a, const name& b ) { return a.value == b.value; }

      /**
       * Internal Representation of the account name
       * 
       * @brief Internal Representation of the account name
       */
      account_name value = 0;
   };

} // namespace eosio

namespace std {
   /**
    *  Provide less for checksum256
    *  @brief Provide less for checksum256
    */
   template<>
   struct less<checksum256> : binary_function<checksum256, checksum256, bool> {
      bool operator()( const checksum256& lhs, const checksum256& rhs ) const {
         return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
      }
   };

} // namespace std

/**
 * Equality Operator for checksum256
 * 
 * @brief Equality Operator for checksum256
 * @param lhs - First data to be compared
 * @param rhs - Second data to be compared
 * @return true - if equal 
 * @return false - if unequal
 */
bool operator==(const checksum256& lhs, const checksum256& rhs) {
   return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}


