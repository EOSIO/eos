/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosiolib/types.h>

extern "C" {

   /**
    * @defgroup systemapi System API
    * @ingroup contractdev
    * @brief Defines API for interacting with system level intrinsics
    *
    */

   /**
    * @defgroup systemcapi System C API
    * @ingroup systemapi
    * @brief Defines API for interacting with system level intrinsics
    *
    * @{
    */

   /**
    *  Aborts processing of this action and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this action and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *
    *  Example:
*
    *  @code
    *  eosio_assert(1 == 2, "One is not equal to two.");
    *  eosio_assert(1 == 1, "One is not equal to one.");
    *  @endcode
    *
    *  @param msg - a null terminated string explaining the reason for failure
    */
   void  eosio_assert( uint32_t test, const char* msg );

   /**
    *  Aborts processing of this action and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this action and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *  @param msg - a pointer to the start of string explaining the reason for failure
    *  @param msg_len - length of the string
    */
   void  eosio_assert_message( uint32_t test, const char* msg, uint32_t msg_len );

   /**
    *  Aborts processing of this action and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this action and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *  @param code - the error code
    */
   void  eosio_assert_code( uint32_t test, uint64_t code );

    /**
    *  This method will abort execution of wasm without failing the contract. This is used to bypass all cleanup / destructors that would normally be called.
    *  @brief Aborts execution of wasm without failing the contract
    *  @param code - the exit code
    *  Example:
*
    *  @code
    *  eosio_exit(0);
    *  eosio_exit(1);
    *  eosio_exit(2);
    *  eosio_exit(3);
    *  @endcode
    */
   [[noreturn]] void  eosio_exit( int32_t code );


   /**
    *  Returns the time in microseconds from 1970 of the current block
    *  @brief Get time of the current block (i.e. the block including this action)
    *  @return time in microseconds from 1970 of the current block
    */
   uint64_t  current_time();

   /**
    *  Returns the time in seconds from 1970 of the block including this action
    *  @brief Get time (rounded down to the nearest second) of the current block (i.e. the block including this action)
    *  @return time in seconds from 1970 of the current block
    */
   uint32_t  now() {
      return (uint32_t)( current_time() / 1000000 );
   }
   ///@ } systemcapi


}
