/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/print.h>
#include <eosiolib/types.hpp>
#include <eosiolib/math.hpp>

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
      printi(num);
   }

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
      printi128((uint128_t*)&num);
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
   void print( Arg a, Args... args ) {
      print(a);
      print(args...);
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
