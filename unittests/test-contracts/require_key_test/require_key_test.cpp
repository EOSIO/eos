#include <eosio/eosio.hpp>
#include <eosio/crypto.hpp>

namespace eosio {
   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((eosio_wasm_import))
         void require_key(void*, size_t);
      }
   }
}

class [[eosio::contract]] require_key_test : public eosio::contract {
public:
   using eosio::contract::contract;

   void require_key( eosio::public_key pk ) {
      char buffer[256];
      eosio::datastream<char*> ds(buffer, 256);
      ds << pk;
      eosio::internal_use_do_not_use::require_key(buffer, ds.tellp());
   }
   [[eosio::action]]
   void reqkey( eosio::public_key pk ) {
      require_key(pk);
   }

   [[eosio::action]]
   void reqkey2( eosio::public_key pk1, eosio::public_key pk2 ) {
      require_key(pk1);
      require_key(pk2);
   }
};
