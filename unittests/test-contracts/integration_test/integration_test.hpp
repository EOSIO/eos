#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] integration_test : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void store( apifiny::name from, apifiny::name to, uint64_t num );

   struct [[apifiny::table("payloads")]] payload {
      uint64_t              key;
      std::vector<uint64_t> data;

      uint64_t primary_key()const { return key; }

      EOSLIB_SERIALIZE( payload, (key)(data) )
   };

   using payloads_table = apifiny::multi_index< "payloads"_n,  payload >;

};
