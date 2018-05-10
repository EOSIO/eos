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
    *  @param cstr - a null terminated action to explain the reason for failure
    *  Example:
    *  @code
    *  eosio_assert(1 == 2, "One is not equal to two.");
    *  eosio_assert(1 == 1, "One is not equal to one.");
    *  @endcode
    */
   void  eosio_assert( uint32_t test, const char* cstr );

    /**
    *  This method will abort execution of wasm without failing the contract. This is used to bypass all cleanup / destructors that would normally be called.
    *  @brief Aborts execution of wasm without failing the contract
    *  @param code - the exit code
    *  Example:
    *  @code
    *  eosio_exit(0);
    *  eosio_exit(1);
    *  eosio_exit(2);
    *  eosio_exit(3);
    *  @endcode
    */
   [[noreturn]] void  eosio_exit( int32_t code );

   /**
    *  Returns the time in seconds from 1970 of the last accepted block (not the block including this action)
    *  @brief Get time of the last accepted block
    *  @return time in seconds from 1970 of the last accepted block
    *  Example:
    *  @code
    *  time current_time = now();
    *  @endcode
    */
   time  now();
   ///@ } systemcapi


}

