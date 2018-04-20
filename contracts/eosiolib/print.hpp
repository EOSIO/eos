/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/print.h>
#include <eosiolib/types.hpp>
#include <eosiolib/math.hpp>
#include <eosiolib/fixed_key.hpp>
#include <utility>

namespace eosio {

   static_assert( sizeof(long) == sizeof(int), "unexpected size difference" );

   /**
    *  Prints string
    *  @brief Prints string
    *  @param ptr - a null terminated string
    */
   inline void print( const char* ptr ) {
      prints(ptr);
   }

   /**
    * Prints 64 bit unsigned integer as a 64 bit unsigned integer
    * @brief Prints integer 64 bit unsigned integer
    * @param num to be printed
    */
   inline void print( uint64_t num ) {
      printui(num);
   }
   inline void print( int64_t num ) {
      printi(num);
   }
   inline void print( double d ) { printdf( d ); }
   inline void print( float f ) { printff( f ); }

   /**
    * Prints 32 bit unsigned integer as a 64 bit unsigned integer
    * @brief Prints integer  32 bit unsigned integer
    * @param num to be printed
    */
   inline void print( uint32_t num ) {
      printi(num);
   }

   /**
    * Prints integer as a 64 bit unsigned integer
    * @brief Prints integer
    * @param num to be printed
    */
   inline void print( int num ) {
      printi(num);
   }

   inline void print( long num ) {
      printi(num);
   }

   /**
    * Prints unsigned integer as a 64 bit unsigned integer
    * @brief Prints unsigned integer
    * @param num to be printed
    */
   inline void print( unsigned int num ) {
      printi(num);
   }

   /**
    * Prints uint128 struct as 128 bit unsigned integer
    * @brief Prints uint128 struct
    * @param num to be printed
    */
   inline void print( uint128 num ) {
      printi128((uint128_t*)&num);
   }

   /**
    * Prints 128 bit unsigned integer
    * @brief Prints 128 bit unsigned integer
    * @param num to be printed
    */
   inline void print( uint128_t num ) {
      printi128(&num);
   }


   /**
    * Prints fixed_key as a hexidecimal string
    * @brief Prints fixed_key as a hexidecimal string
    * @param val to be printed
    */
   template<size_t Size>
   inline void print( const fixed_key<Size>& val ) {
      auto arr = val.extract_as_byte_array();
      prints("0x");
      printhex(static_cast<const void*>(arr.data()), arr.size());
   }

   template<size_t Size>
   inline void print( fixed_key<Size>& val ) {
      print(static_cast<const fixed_key<Size>&>(val));
   }

   /**
    * Prints a 64 bit names as base32 encoded string
    * @brief Prints a 64 bit names as base32 encoded string
    * @param name 64 bit name to be printed
    */
   inline void print( name name ) {
      printn(name.value);
   }

   inline void print( bool val ) {
      prints(val?"true":"false");
   }


   template<typename T>
   inline void print( T&& t ) {
      t.print();
   }


   inline void print_f( const char* s ) {
      prints(s);
   }

   template <typename Arg, typename... Args>
   inline void print_f( const char* s, Arg val, Args... rest ) {
      while ( *s != '\0' ) {
         if ( *s == '%' ) {
            print( val );
            print_f( s+1, rest... );
            return;
         }
         prints_l( s, 1 );
         s++;
      }
   }


   /**
    *  @defgroup consoleCppapi Console C++ API
    *  @ingroup consoleapi
    *  @brief C++ wrapper for Console C API
    *
    *  This API uses C++ variadic templates and type detection to
    *  make it easy to print any native type. You can even overload
    *  the `print()` method for your own custom types.
    *
    *  **Example:**
    *  ```
    *     print( "hello world, this is a number: ", 5 );
    *  ```
    *
    *  @section override Overriding Print for your Types
    *
    *  There are two ways to overload print:
    *  1. implement void print( const T& )
    *  2. implement T::print()const
    *
    *  @{
    */

    /**
     *  Print out value / list of values (except double)
     *  @brief Print out value  / list of values
     *  @param a    Value to be printed
     *  @param args Other values to be printed
     *
     *  Example:
     *  @code
     *  const char *s = "Hello World!";
     *  uint64_t unsigned_64_bit_int = 1e+18;
     *  uint128_t unsigned_128_bit_int (87654323456);
     *  uint64_t string_as_unsigned_64_bit = N(abcde);
     *  print(s , unsigned_64_bit_int, unsigned_128_bit_int, string_as_unsigned_64_bit);
     *  // Ouput: Hello World!100000000000000000087654323456abcde
     *  @endcode
     */
   template<typename Arg, typename... Args>
   void print( Arg&& a, Args&&... args ) {
      print(std::forward<Arg>(a));
      print(std::forward<Args>(args)...);
   }

   /**
    * Simulate C++ style streams
    */
   class iostream {};

   /**
    *  Overload c++ iostream
    *  @brief Overload c++ iostream
    *  @param out  Output strem
    *  @param v    Value to be printed
    *
    *  Example:
    *  @code
    *  const char *s = "Hello World!";
    *  uint64_t unsigned_64_bit_int = 1e+18;
    *  uint128_t unsigned_128_bit_int (87654323456);
    *  uint64_t string_as_unsigned_64_bit = N(abcde);
    *  std::out << s << " " << unsigned_64_bit_int << " "  << unsigned_128_bit_int << " " << string_as_unsigned_64_bit;
    *  // Output: Hello World! 1000000000000000000 87654323456 abcde
    *  @endcode
    */
   template<typename T>
   inline iostream& operator<<( iostream& out, const T& v ) {
      print( v );
      return out;
   }

   static iostream cout;

   /// @} consoleCppapi


}
