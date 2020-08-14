#pragma once

#include <eosio/eosio.hpp>

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

   [[eosio::on_notify("*::notify")]]
   void on_notify( eosio::name to, eosio::name expected_sender, bool send_inline );

};
