#pragma once

#include <eosio/eosio.hpp>

namespace eosio {
   namespace internal_use_do_not_use {
      extern "C" {
         struct __attribute__((aligned (16))) capi_checksum256 { uint8_t hash[32]; };
         __attribute__((eosio_wasm_import))
         void get_code_version(uint64_t name, int64_t* last_code_update_time, capi_checksum256* code_hash);
      }
   }
}

namespace eosio {
   time_point get_code_last_update_time(name name) {
      time_point last_code_update;
      internal_use_do_not_use::capi_checksum256 code_hash;
      internal_use_do_not_use::get_code_version(name.value, &last_code_update.elapsed._count, &code_hash);
      return last_code_update;
   }
   checksum256 get_code_version(name name) {
      time_point last_code_update;
      internal_use_do_not_use::capi_checksum256 code_hash;
      internal_use_do_not_use::get_code_version(name.value, &last_code_update.elapsed._count, &code_hash);
      return {code_hash.hash};
   }
}

class [[eosio::contract]] code_version_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void reqversion( eosio::name name, eosio::checksum256 version );

   [[eosio::action]]
   void reqtime( eosio::name name, eosio::time_point last_code_update );
};
