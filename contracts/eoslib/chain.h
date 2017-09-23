#pragma once

#include <eoslib/types.h>

/**
 *  @defgroup chaincapi Chain API
 *  @brief Define API for querying internal chain state
 *  @ingroup contractdev
 */

extern "C" {
   /**
    * @brief Return the set of active producers
    *
    * @param producers - location to store the active producers
    *
    *  Example:
    *  @code
    *  AccountName producers[21];
    *  getActiveProducers(producers, sizeof(AccountName)*21);
    *  @endcode
    */

   void getActiveProducers( AccountName* producers, uint32_t datalen );

   ///@ } chaincapi
}