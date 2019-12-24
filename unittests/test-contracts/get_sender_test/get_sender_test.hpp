#pragma once

#include <apifiny/apifiny.hpp>

namespace apifiny {
   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((apifiny_wasm_import))
         uint64_t get_sender();
      }
   }
}

namespace apifiny {
   name get_sender() {
      return name( internal_use_do_not_use::get_sender() );
   }
}

class [[apifiny::contract]] get_sender_test : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void assertsender( apifiny::name expected_sender );
   using assertsender_action = apifiny::action_wrapper<"assertsender"_n, &get_sender_test::assertsender>;

   [[apifiny::action]]
   void sendinline( apifiny::name to, apifiny::name expected_sender );

   [[apifiny::action]]
   void notify( apifiny::name to, apifiny::name expected_sender, bool send_inline );

   // apifiny.cdt 1.6.1 has a problem with "*::notify" so hardcode to tester1 for now.
   // TODO: Change it back to "*::notify" when the bug is fixed in apifiny.cdt.
   [[apifiny::on_notify("tester1::notify")]]
   void on_notify( apifiny::name to, apifiny::name expected_sender, bool send_inline );

};
