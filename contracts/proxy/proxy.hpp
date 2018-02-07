/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

namespace proxy {
   
   //@abi action
   struct PACKED( set_owner ) {
      account_name owner;
      uint32_t     delay;
   };

   //@abi table
   struct config {
      config(){}
      const uint64_t      key = N(config);
      account_name        owner = 0;
      uint32_t            delay = 0;
      uint32_t            next_id = 0;
   };

} /// namespace proxy
