#pragma once
#include <eoslib/types.h>
  
namespace  eos {

   /**
    *  Converts a base32 symbol into its binary representation, used by string_to_name()
    */
   static constexpr char char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 1;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 27;
      return 0;
   }

   /**
    *  Converts a base32 string to a uint64_t. This is a constexpr so that
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
   #define N(X) ::eos::string_to_name(#X)

   /**
    *  @class Name
    *  @brief wraps a uint64_t to ensure it is only passed to methods that expect a Name and
    *         that no mathematical operations occur.  It also enables specialization of print
    *         so that it is printed as a base32 string.
    *
    *  @ingroup types
    */
   struct Name {
      Name( uint64_t v = 0 ): value(v) {}
      operator uint64_t()const { return value; }

      friend bool operator==( const Name& a, const Name& b ) { return a.value == b.value; }
      AccountName value = 0;
   };


   /**
    * @ingroup types
    *
    * @{
    */
   template<typename T> struct remove_reference           { typedef T type; };
   template<typename T> struct remove_reference<T&>       { typedef T type; };
   template<typename T> struct remove_reference<const T&> { typedef T type; };
   ///@}

} // namespace eos
