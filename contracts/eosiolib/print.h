/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/types.h>

#ifdef __cplusplus
extern "C" {
#endif
   /**
    *  @defgroup consoleapi Console API
    *  @brief Enables applications to log/print text messages
    *  @ingroup contractdev
    *
    */

   /**
    *  @defgroup consolecapi Console C API
    *  @brief C API to log/print text messages
    *  @ingroup consoleapi
    *  @{
    */

   /**
    *  Prints string
    *  @brief Prints string
    *  @param cstr - a null terminated string
    *
    *  Example:
    *  @code
    *  prints("Hello World!"); // Output: Hello World!
    *  @endcode
    */
   void prints( const char* cstr );

   /**
    *  Prints string up to given length
    *  @brief Prints string
    *  @param cstr - pointer to string
    *  @param len - len of string to be printed
    *
    *  Example:
    *  @code
    *  prints_l("Hello World!", 5); // Output: Hello
    *  @endcode
    */
   void prints_l( const char* cstr, uint32_t len);

   /**
    * Prints value as a 64 bit unsigned integer
    * @brief Prints value as a 64 bit unsigned integer
    * @param value of 64 bit unsigned integer to be printed
    *
    *  Example:
    *  @code
    *  printi(1e+18); // Output: 1000000000000000000
    *  @endcode
    */
   void printi( uint64_t value );

   /**
    * Prints value as a 128 bit unsigned integer
    * @brief Prints value as a 128 bit unsigned integer
    * @param value 128 bit integer to be printed
    *
    *  Example:
    *  @code
    *  uint128_t large_int(87654323456);
    *  printi128(large_int); // Output: 87654323456
    *  @endcode
    */
   void printi128( const uint128_t* value );

   /**
    * Prints value as double
    * @brief Prints value as double
    * @param value of double (interpreted as 64 bit unsigned integer) to be printed
    *
    *  Example:
    *  @code
    *  uint64_t double_value = double_div( i64_to_double(5), i64_to_double(10) );
    *  printd(double_value); // Output: 0.5
    *  @endcode
    */
   void printd(uint64_t value);

   /**
    * Prints a 64 bit names as base32 encoded string
    * @brief Prints a 64 bit names as base32 encoded string
    * @param name - 64 bit name to be printed
    *
    * Example:
    * @code
    * printn(N(abcde)); // Output: abcde
    * @endcode
    */
   void printn( uint64_t name );

   /**
    */
   void printhex( void* data, uint32_t datalen );
   /// @}
#ifdef __cplusplus
}
#endif
