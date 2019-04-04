/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] integration_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void store( eosio::name from, eosio::name to, uint64_t num );

   struct [[eosio::table("payloads")]] payload {
      uint64_t              key;
      std::vector<uint64_t> data;

      uint64_t primary_key()const { return key; }

      EOSLIB_SERIALIZE( payload, (key)(data) )
   };

   using payloads_table = eosio::multi_index< "payloads"_n,  payload >;

};
