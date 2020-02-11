#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] payloadless : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void doit();
};
