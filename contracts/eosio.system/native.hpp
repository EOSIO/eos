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

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   struct key_weight {
      public_key   key;
      weight_type  weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   struct authority {
      uint32_t                              threshold;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts) )
   };

   /*
    * Empty handlers for native messages.
    * Method parameters commented out to prevent generation of code that parses input data.
    */
   class native {
      public:

      void newaccount( /*account_name     creator,
                              account_name     name,
                              const authority& owner,
                              const authority& active,
                              const authority& recovery*/ ) {}

      void updateauth( /*account_name     account,
                              permission_name  permission,
                              permission_name  parent,
                              const authority& data*/ ) {}

      void deleteauth( /*account_name account, permission_name permission*/ ) {}

      void linkauth( /*account_name    account,
                            account_name    code,
                            action_name     type,
                            permission_name requirement*/ ) {}

      void unlinkauth( /*account_name account,
                              account_name code,
                              action_name  type*/ ) {}

      void postrecovery( /*account_name       account,
                                const authority&   data,
                                const std::string& memo*/ ) {}

      void passrecovery( /*account_name account*/ ) {}

      void vetorecovery( /*account_name account*/ ) {}

      void onerror( /*const bytes&*/ ) {}

      void canceldelay( /*permission_level canceling_auth, transaction_id_type trx_id*/ ) {}

   };
}
