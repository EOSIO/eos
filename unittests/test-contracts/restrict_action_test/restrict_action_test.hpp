#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] restrict_action_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void noop( );

   [[eosio::action]]
   void sendinline( eosio::name authorizer );

   [[eosio::action]]
   void senddefer( eosio::name authorizer, uint32_t senderid );


   [[eosio::action]]
   void notifyinline( eosio::name acctonotify, eosio::name authorizer );

   [[eosio::action]]
   void notifydefer( eosio::name acctonotify, eosio::name authorizer, uint32_t senderid );

   [[eosio::on_notify("testacc::notifyinline")]]
   void on_notify_inline( eosio::name acctonotify, eosio::name authorizer );

   [[eosio::on_notify("testacc::notifydefer")]]
   void on_notify_defer( eosio::name acctonotify, eosio::name authorizer, uint32_t senderid );
};
