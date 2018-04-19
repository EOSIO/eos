/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/action.hpp>
#include <eosiolib/public_key.hpp>
#include <eosiolib/types.hpp>

namespace eosiosystem {
   using eosio::permission_level;
   using eosio::public_key;

   typedef std::vector<char> bytes;

   struct permission_level_weight {
      permission_level  permission;
      weight_type       weight;

      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   struct key_weight {
      public_key   key;
      weight_type  weight;

      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   struct authority {
      uint32_t                              threshold;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;

      EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts) )
   };

   class native {
      public:

      static void newaccount( account_name     creator,
                              account_name     name,
                              const authority& owner,
                              const authority& active,
                              const authority& recovery ) {}

      static void updateauth( account_name     account,
                              permission_name  permission,
                              permission_name  parent,
                              const authority& data ) {}

      static void deleteauth( account_name account, permission_name permission ) {}

      static void linkauth( account_name    account,
                            account_name    code,
                            action_name     type,
                            permission_name requirement ) {}

      static void unlinkauth( account_name account,
                              account_name code,
                              action_name  type ) {}

      static void postrecovery( account_name       account,
                                const authority&   data,
                                const std::string& memo ) {}

      static void passrecovery( account_name account ) {}

      static void vetorecovery( account_name   account ) {}

      static void onerror( const bytes& ) {}

      static void canceldelay( permission_level canceling_auth, transaction_id_type trx_id ) {}

   };
}
