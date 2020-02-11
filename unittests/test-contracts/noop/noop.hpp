/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] noop : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void anyaction( eosio::name                       from,
                   const eosio::ignore<std::string>& type,
                   const eosio::ignore<std::string>& data );
};
