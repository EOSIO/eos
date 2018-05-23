#pragma once

#include "common.hpp"

namespace identity {

   class interface : public identity_base {
      public:
         using identity_base::identity_base;

         identity_name get_claimed_identity( eosio::account_name acnt );

         eosio::account_name get_owner_for_identity( uint64_t receiver, identity_name ident );

         identity_name get_identity_for_account( eosio::account_name receiver, eosio::account_name acnt );
   };

}
