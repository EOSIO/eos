#pragma once

#include <eosio/eosio.hpp>

using namespace eosio;

extern "C" {
   using capi_checksum256 = std::array<uint8_t,32> __attribute__ ((aligned(16)));
   struct capi_code_version {
      capi_checksum256 hash;
      int64_t last_updated;
   };
   __attribute__((eosio_wasm_import))
   uint32_t get_code_version(uint64_t, char*, uint32_t);
}

time_point get_code_last_updated(name name) {
   char buffer[40];
   get_code_version(name.value, buffer, sizeof(buffer));

   capi_code_version code_ver;
   datastream<const char*> ds(buffer, sizeof(buffer));
   ds >> code_ver;
   return time_point(microseconds(code_ver.last_updated));
}
checksum256 get_code_version(name name) {
   char buffer[40];
   get_code_version(name.value, buffer, sizeof(buffer));

   capi_code_version code_ver;
   datastream<const char*> ds(buffer, sizeof(buffer));
   ds >> code_ver;
   return checksum256(code_ver.hash);
}

class [[eosio::contract]] code_version_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void reqversion( eosio::name name, eosio::checksum256 version );

   [[eosio::action]]
   void reqtime( eosio::name name, eosio::time_point last_updated );
};
