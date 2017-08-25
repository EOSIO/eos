#pragma once

extern "C" {
   /**
    *  @defgroup consoleapi Console API
    *  @brief Enables applications to log/print text messages
    *  @ingroup contractdev
    *
    */

   /**
    *  @defgroup consolecapi Console C API
    *  @ingroup consoleapi
    *
    *  @{
    */

   /**
    *  Prints string
    *  @brief Prints string
    *  @param cstr - a null terminated string
    */
   void prints( const char* cstr );

   /**
    * Prints value as a 64 bit unsigned integer
    * @brief Prints value as a 64 bit unsigned integer
    * @param Value of 64 bit unsigned integer to be printed
    */
   void printi( uint64_t value );

   /**
    * Prints value as a 128 bit unsigned integer
    * @brief Prints value as a 128 bit unsigned integer
    * @param value 128 bit integer to be printed
    */
   void printi128( const uint128_t* value );

   /**
    * Prints value as double
    * @brief Prints value as double
    * @param Value of double (interpreted as 64 bit unsigned integer) to be printed
    */
   void printd(uint64_t value);

   /**
    * Prints a 64 bit names as base32 encoded string
    * @brief Prints a 64 bit names as base32 encoded string
    * @param Value of 64 bit names to be printed
    */
   void printn( uint64_t name );
   /// @}
}
