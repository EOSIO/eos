/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosiolib/print.h>
#include <eosiolib/types.hpp>
#include <eosiolib/fixed_key.hpp>
#include <utility>
#include <string>

namespace eosio {

   static_assert( sizeof(long) == sizeof(int), "unexpected size difference" );

   /**
    *  Prints string
    * 
    *  @brief Prints string
    *  @param ptr - a null terminated string
    */
   inline void print( const char* ptr ) {
      prints(ptr);
   }

   inline void print( const std::string& s) {
      prints_l( s.c_str(), s.size() );
   }

   inline void print( std::string& s) {
      prints_l( s.c_str(), s.size() );
   }

   inline void print( const char c ) {
      prints_l( &c, 1 );
   }

   /**
    * Prints signed integer
    * 
    * @brief Prints signed integer as a 64 bit signed integer
    * @param num to be printed
    */
   inline void print( int num ) {
      printi(num);
   }

   /**
    * Prints 32 bit signed integer
    * 
    * @brief Prints 32 bit signed integer as a 64 bit signed integer
    * @param num to be printed
    */
   inline void print( int32_t num ) {
      printi(num);
   }

   /**
    * Prints 64 bit signed integer
    * 
    * @brief Prints 64 bit signed integer as a 64 bit signed integer
    * @param num to be printed
    */
   inline void print( int64_t num ) {
      printi(num);
   }


   /**
    * Prints unsigned integer
    * 
    * @brief Prints unsigned integer as a 64 bit unsigned integer
    * @param num to be printed
    */
   inline void print( unsigned int num ) {
      printui(num);
   }

   /**
    * Prints 32 bit unsigned integer
    * 
    * @brief Prints 32 bit unsigned integer as a 64 bit unsigned integer
    * @param num to be printed
    */
   inline void print( uint32_t num ) {
      printui(num);
   }

   /**
    * Prints 64 bit unsigned integer
    * 
    * @brief Prints 64 bit unsigned integer as a 64 bit unsigned integer
    * @param num to be printed
    */
   inline void print( uint64_t num ) {
      printui(num);
   }

   /**
    * Prints 128 bit signed integer
    * 
    * @brief Prints 128 bit signed integer
    * @param num to be printed
    */
   inline void print( int128_t num ) {
      printi128(&num);
   }

   /**
    * Prints 128 bit unsigned integer
    * 
    * @brief Prints 128 bit unsigned integer
    * @param num to be printed
    */
   inline void print( uint128_t num ) {
      printui128(&num);
   }


   /**
    * Prints single-precision floating point number
    * 
    * @brief Prints single-precision floating point number (i.e. float)
    * @param num to be printed
    */
   inline void print( float num ) { printsf( num ); }

   /**
    * Prints double-precision floating point number
    * 
    * @brief Prints double-precision floating point number (i.e. double)
    * @param num to be printed
    */
   inline void print( double num ) { printdf( num ); }

   /**
    * Prints quadruple-precision floating point number
    * 
    * @brief Prints quadruple-precision floating point number (i.e. long double)
    * @param num to be printed
    */
   inline void print( long double num ) { printqf( &num ); }


   /**
    * Prints fixed_key as a hexidecimal string
    * 
    * @brief Prints fixed_key as a hexidecimal string
    * @param val to be printed
    */
   template<size_t Size>
   inline void print( const fixed_key<Size>& val ) {
      auto arr = val.extract_as_byte_array();
      prints("0x");
      printhex(static_cast<const void*>(arr.data()), arr.size());
   }

  /**
    * Prints fixed_key as a hexidecimal string
    * 
    * @brief Prints fixed_key as a hexidecimal string
    * @param val to be printed
    */
   template<size_t Size>
   inline void print( fixed_key<Size>& val ) {
      print(static_cast<const fixed_key<Size>&>(val));
   }

   /**
    * Prints a 64 bit names as base32 encoded string
    * 
    * @brief Prints a 64 bit names as base32 encoded string
    * @param name 64 bit name to be printed
    */
   inline void print( name name ) {
      printn(name.value);
   }

  /**
    * Prints bool
    * 
    * @brief Prints bool
    * @param val to be printed
    */
   inline void print( bool val ) {
      prints(val?"true":"false");
   }


  /**
    * Prints class object
    * 
    * @brief Prints class object
    * @param t to be printed
    * @pre T must implements print() function
    */
   template<typename T>
   inline void print( T&& t ) {
      t.print();
   }

   /**
    * Prints null terminated string
    * 
    * @brief Prints null terminated string
    * @param s null terminated string to be printed
    */
   inline void print_f( const char* s ) {
      prints(s);
   }

 /**
    *  @defgroup consolecppapi Console C++ API
    *  @ingroup consoleapi
    *  @brief Defines C++ wrapper to log/print text messages
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
    * Prints formatted string. It behaves similar to C printf/
    * 
    * @brief Prints formatted string
    * @tparam Arg - Type of the value used to replace the format specifier
    * @tparam Args - Type of the value used to replace the format specifier
    * @param s - Null terminated string with to be printed (it can contains format specifier)
    * @param val - The value used to replace the format specifier
    * @param rest - The values used to replace the format specifier
    * 
    * Example:
    * @code
    * print_f("Number of apples: %", 10);
    * @endcode
    */
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
     *  Print out value / list of values 
     *  @brief Print out value  / list of values
     *  @param a - The value to be printed
     *  @param args - The other values to be printed
     *
     *  Example:
*
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
    *  @param out - Output strem
    *  @param v - The value to be printed
    *  @return iostream& - Reference to the input output stream
    *
    *  Example:
*
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

   /// @} consolecppapi


}
