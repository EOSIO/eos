#pragma once
#include <eos/chain/protocol/types.hpp>


namespace eos { namespace chain {

   struct PermissionLevel {
      account_name      account;
      permission_name   level;
      uint16_t          weight;
   };

   struct KeyPermission {
      public_key_type key;
      uint16_t        weight;
   };

   struct Authority {
      uint32_t                threshold = 0;
      vector<PermissionLevel> accounts;
      vector<KeyPermission>   keys;
   };

} }  // eos::chain

FC_REFLECT( eos::chain::Authority, (threshold)(accounts)(keys) )
