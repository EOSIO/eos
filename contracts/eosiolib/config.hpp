#pragma once

#include <eosiolib/types.hpp>

namespace eosio { namespace cfg {

/**
 * @defgroup configtype Config Type
 * @ingroup types
 * @brief Defines chain configuration constants
 * 
 * @{
 * 
 */

   /**
    * The account name of the system
    * @brief The account name of the system
    */
   const static uint64_t system_account_name = N(eosio);

/// @} configtype
} /// namespace cfg
} /// namespace eosio
