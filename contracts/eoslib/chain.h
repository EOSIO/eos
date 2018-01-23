/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eoslib/types.h>

/**
 *  @defgroup chainapi Chain API
 *  @brief Define API for querying internal chain state
 *  @ingroup contractdev
 */

/**
 *  @defgroup chaincapi Chain C API
 *  @brief C API for querying internal chain state
 *  @ingroup chainapi
 *  @{
 */

extern "C" {
   /**
    * Return the set of active producers
    * @param producers - a pointer to an buffer of account_names
    * @param datalen - byte length of buffer
    * @return the number of bytes actually populated
    *
    *  Example:
    *  @code
    *  account_name producers[21];
    *  uint32_t bytes_populated = get_active_producers(producers, sizeof(account_name)*21);
    *  @endcode
    */

   uint32_t get_active_producers( account_name* producers, uint32_t datalen );

   ///@ } chaincapi
}
