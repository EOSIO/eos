/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosiolib/types.h>
#include <functional>
#include <tuple>
#include <string>

namespace eosio {

   typedef std::vector<std::tuple<uint16_t,std::vector<char>>> extensions_type;

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


   static constexpr uint64_t name_suffix( uint64_t n ) {
      uint32_t remaining_bits_after_last_actual_dot = 0;
      uint32_t tmp = 0;
      for( int32_t remaining_bits = 59; remaining_bits >= 4; remaining_bits -= 5 ) { // Note: remaining_bits must remain signed integer
         // Get characters one-by-one in name in order from left to right (not including the 13th character)
         auto c = (n >> remaining_bits) & 0x1Full;
         if( !c ) { // if this character is a dot
            tmp = static_cast<uint32_t>(remaining_bits);
         } else { // if this character is not a dot
            remaining_bits_after_last_actual_dot = tmp;
         }
      }

      uint64_t thirteenth_character = n & 0x0Full;
      if( thirteenth_character ) { // if 13th character is not a dot
         remaining_bits_after_last_actual_dot = tmp;
      }

      if( remaining_bits_after_last_actual_dot == 0 ) // there is no actual dot in the name other than potentially leading dots
         return n;

      // At this point remaining_bits_after_last_actual_dot has to be within the range of 4 to 59 (and restricted to increments of 5).

      // Mask for remaining bits corresponding to characters after last actual dot, except for 4 least significant bits (corresponds to 13th character).
      uint64_t mask = (1ull << remaining_bits_after_last_actual_dot) - 16;
      uint32_t shift = 64 - remaining_bits_after_last_actual_dot;

      return ( ((n & mask) << shift) + (thirteenth_character << (shift-1)) );
   }

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

      // keep in sync with name::operator string() in eosio source code definition for name
      std::string to_string() const {
         static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

         std::string str(13,'.');

         uint64_t tmp = value;
         for( uint32_t i = 0; i <= 12; ++i ) {
            char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
            str[12-i] = c;
            tmp >>= (i == 0 ? 4 : 5);
         }

         trim_right_dots( str );
         return str;
      }

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

   private:
      static void trim_right_dots(std::string& str ) {
         const auto last = str.find_last_not_of('.');
         if (last != std::string::npos)
            str = str.substr(0, last + 1);
      }
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

/**
 * Equality Operator for checksum160
 *
 * @brief Equality Operator for checksum256
 * @param lhs - First data to be compared
 * @param rhs - Second data to be compared
 * @return true - if equal
 * @return false - if unequal
 */
bool operator==(const checksum160& lhs, const checksum160& rhs) {
   return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

/**
 * Equality Operator for checksum160
 *
 * @brief Equality Operator for checksum256
 * @param lhs - First data to be compared
 * @param rhs - Second data to be compared
 * @return true - if unequal
 * @return false - if equal
 */
bool operator!=(const checksum160& lhs, const checksum160& rhs) {
   return memcmp(&lhs, &rhs, sizeof(lhs)) != 0;
}
