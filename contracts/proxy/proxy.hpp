/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */
#pragma once

#include <enumivolib/enumivo.hpp>

namespace proxy {

   //@abi action
   struct set_owner {
      account_name owner;
      uint32_t     delay;

      ENULIB_SERIALIZE( set_owner, (owner)(delay) )
   };

   //@abi table
   struct config {
      config(){}
      constexpr static uint64_t key = N(config);
      account_name        owner = 0;
      uint32_t            delay = 0;
      uint32_t            next_id = 0;
   };

} /// namespace proxy
