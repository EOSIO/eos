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
    * @brief Define API for interating with system level intrinsics
    *
    */

   /**
    * @defgroup systemcapi System C API
    * @ingroup systemapi
    * @brief Define API for interating with system level intrinsics
    *
    * @{
    */

   /**
    *  Aborts processing of this action and unwinds all pending changes if the test condition is true
    *  @brief Aborts processing of this action and unwinds all pending changes
    *  @param test - 0 to abort, 1 to ignore
    *  @param cstr - a null terminated action to explain the reason for failure

    */
   void  assert( uint32_t test, const char* cstr );

   /**
    *  Returns the time in seconds from 1970 of the last accepted block (not the block including this action)
    *  @brief Get time of the last accepted block
    *  @return time in seconds from 1970 of the last accepted block
    */
   time  now();
   ///@ } systemcapi

   /**
    * @defgroup privilegedapi Privileged API
    * @ingroup systemapi
    * @brief Defines an API for accessing configuration of the chain that can only be done by privileged accounts
    */

   /**
    * @defgroup privilegedcapi Privileged C API
    * @ingroup privilegedapi
    * @brief Define C Privileged API
    *
    * @{
    */

   void set_resource_limits( account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight, int64_t ignored);

   void set_active_producers( char *producer_data, size_t producer_data_size );

   ///@ } privilegedcapi

}

