#pragma once

#include <eosio/eosio.hpp>

namespace eosio {
   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((eosio_wasm_import))
         uint64_t get_sender();
      }
   }
}

namespace eosio {
   name get_sender() {
      return name( internal_use_do_not_use::get_sender() );
   }
}

class [[eosio::contract]] get_sender_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void assertsender( eosio::name expected_sender );
   using assertsender_action = eosio::action_wrapper<"assertsender"_n, &get_sender_test::assertsender>;

   [[eosio::action]]
   void sendinline( eosio::name to, eosio::name expected_sender );

   [[eosio::action]]
   void notify( eosio::name to, eosio::name expected_sender, bool send_inline );

   // eosio.cdt 1.6.1 has a problem with "*::notify" so hardcode to tester1 for now.
   // TODO: Change it back to "*::notify" when the bug is fixed in eosio.cdt.
   [[eosio::on_notify("tester1::notify")]]
   void on_notify( eosio::name to, eosio::name expected_sender, bool send_inline );

};
