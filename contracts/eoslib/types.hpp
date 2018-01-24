/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.h>

namespace  eosio {

   /**
    *  @brief Converts a base32 symbol into its binary representation, used by string_to_name()
    *
    *  @details Converts a base32 symbol into its binary representation, used by string_to_name()
    *  @ingroup types
    */
   static constexpr char char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      return 0;
   }


   /**
    *  @brief Converts a base32 string to a uint64_t. 
    *
    *  @details Converts a base32 string to a uint64_t. This is a constexpr so that
    *  this method can be used in template arguments as well.
    *
    *  @ingroup types
    */
   static constexpr uint64_t string_to_name( const char* str ) {

      uint32_t len = 0;
      while( str[len] ) ++len;

      uint64_t value = 0;

      for( uint32_t i = 0; i <= 12; ++i ) {
         uint64_t c = 0;
         if( i < len && i <= 12 ) c = char_to_symbol( str[i] );

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
    * @brief used to generate a compile time uint64_t from the base32 encoded string interpretation of X
    * @ingroup types
    */
   #define N(X) ::eosio::string_to_name(#X)

   /**
    *  @brief wraps a uint64_t to ensure it is only passed to methods that expect a Name
    *  @details wraps a uint64_t to ensure it is only passed to methods that expect a Name and
    *         that no mathematical operations occur.  It also enables specialization of print
    *         so that it is printed as a base32 string.
    *
    *  @ingroup types
    *  @{
    */
   struct name {
      name( uint64_t v = 0 ): value(v) {}
      operator uint64_t()const { return value; }

      friend bool operator==( const name& a, const name& b ) { return a.value == b.value; }
      account_name value = 0;

      template<typename DataStream>
      friend DataStream& operator << ( DataStream& ds, const name& v ){
         return ds << v.value;
      }
      template<typename DataStream>
      friend DataStream& operator >> ( DataStream& ds, name& v ){
         return ds >> v.value;
      }
   };

   /// @}

   /**
    * @ingroup types
    *
    * @{
    */
   template<typename T> struct remove_reference           { typedef T type; };
   template<typename T> struct remove_reference<T&>       { typedef T type; };
   template<typename T> struct remove_reference<const T&> { typedef T type; };
   ///@}

   typedef decltype(nullptr) nullptr_t;

   struct true_type  { enum _value { value = 1 }; };
   struct false_type { enum _value { value = 0 }; };

   using ::account_name;

} // namespace eosio
