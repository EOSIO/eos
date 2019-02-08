/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>

namespace test_inline {

   using eosio::name;
   using eosio::action_wrapper;

   class [[eosio::contract]] test_inline : public eosio::contract {
   public:
      using eosio::contract::contract;

      [[eosio::action("reqauth")]]
      void require_authorization( name from );
      using require_auth_action = action_wrapper<"reqauth"_n, &test_inline::require_authorization>;

      [[eosio::action]]
      void forward( name reqauth, name forward_code, name forward_auth );
      using forward_action = action_wrapper<"forward"_n, &test_inline::forward>;
   };

} /// namespace test_inline
