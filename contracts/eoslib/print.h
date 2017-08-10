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
    *  @param cstr - a null terminated string
    */
   void prints( const char* cstr );
   /**
    * Prints value as an integer
    */
   void printi( uint64_t value );

   void printi128( const uint128_t* value );
   void printd(uint64_t value);
   /**
    * Prints a 64 bit names as base32 encoded string
    */
   void printn( uint64_t name );
   /// @}
}
