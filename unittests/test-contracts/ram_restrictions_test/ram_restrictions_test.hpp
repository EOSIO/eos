#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] ram_restrictions_test : public apifiny::contract {
public:
   struct [[apifiny::table]] data {
      uint64_t           key;
      std::vector<char>  value;

      uint64_t primary_key() const { return key; }
   };

   typedef apifiny::multi_index<"tablea"_n, data> tablea;
   typedef apifiny::multi_index<"tableb"_n, data> tableb;

public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void noop();

   [[apifiny::action]]
   void setdata( uint32_t len1, uint32_t len2, apifiny::name payer );

   [[apifiny::action]]
   void notifysetdat( apifiny::name acctonotify, uint32_t len1, uint32_t len2, apifiny::name payer );

   [[apifiny::on_notify("tester2::notifysetdat")]]
   void on_notify_setdata( apifiny::name acctonotify, uint32_t len1, uint32_t len2, apifiny::name payer );

   [[apifiny::action]]
   void senddefer( uint64_t senderid, apifiny::name payer );

   [[apifiny::action]]
   void notifydefer( apifiny::name acctonotify, uint64_t senderid, apifiny::name payer );

   [[apifiny::on_notify("tester2::notifydefer")]]
   void on_notifydefer( apifiny::name acctonotify, uint64_t senderid, apifiny::name payer );

};
