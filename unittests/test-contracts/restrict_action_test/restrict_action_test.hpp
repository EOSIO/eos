#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] restrict_action_test : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void noop( );

   [[apifiny::action]]
   void sendinline( apifiny::name authorizer );

   [[apifiny::action]]
   void senddefer( apifiny::name authorizer, uint32_t senderid );


   [[apifiny::action]]
   void notifyinline( apifiny::name acctonotify, apifiny::name authorizer );

   [[apifiny::action]]
   void notifydefer( apifiny::name acctonotify, apifiny::name authorizer, uint32_t senderid );

   [[apifiny::on_notify("testacc::notifyinline")]]
   void on_notify_inline( apifiny::name acctonotify, apifiny::name authorizer );

   [[apifiny::on_notify("testacc::notifydefer")]]
   void on_notify_defer( apifiny::name acctonotify, apifiny::name authorizer, uint32_t senderid );
};
