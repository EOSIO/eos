/**
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/name.hpp>

using eosio::action_wrapper;
using eosio::asset;
using eosio::name;

class [[eosio::contract("rex.results")]] rex_results : eosio::contract {
   public:

      using eosio::contract::contract;

      [[eosio::action]]
      void buyresult( const asset& rex_received );

      [[eosio::action]]
      void sellresult( const asset& proceeds );

      [[eosio::action]]
      void orderresult( const name& owner, const asset& proceeds );

      [[eosio::action]]
      void rentresult( const asset& rented_tokens );

      using buyresult_action   = action_wrapper<"buyresult"_n,   &rex_results::buyresult>;
      using sellresult_action  = action_wrapper<"sellresult"_n,  &rex_results::sellresult>;
      using orderresult_action = action_wrapper<"orderresult"_n, &rex_results::orderresult>;
      using rentresult_action  = action_wrapper<"rentresult"_n,  &rex_results::rentresult>;
};
