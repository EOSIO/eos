/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] ram_restrictions_test : public eosio::contract {
public:
   struct [[eosio::table]] data {
      uint64_t           key;
      std::vector<char>  value;

      uint64_t primary_key() const { return key; }
   };

   typedef eosio::multi_index<"tablea"_n, data> tablea;
   typedef eosio::multi_index<"tableb"_n, data> tableb;

public:
   using eosio::contract::contract;

   [[eosio::action]]
   void noop();

   [[eosio::action]]
   void setdata( uint32_t len1, uint32_t len2, eosio::name payer );

   [[eosio::action]]
   void notifysetdat( eosio::name acctonotify, uint32_t len1, uint32_t len2, eosio::name payer );

   [[eosio::on_notify("tester2::notifysetdat")]]
   void on_notify_setdata( eosio::name acctonotify, uint32_t len1, uint32_t len2, eosio::name payer );

   [[eosio::action]]
   void senddefer( uint64_t senderid, eosio::name payer );

   [[eosio::action]]
   void notifydefer( eosio::name acctonotify, uint64_t senderid, eosio::name payer );

   [[eosio::on_notify("tester2::notifydefer")]]
   void on_notifydefer( eosio::name acctonotify, uint64_t senderid, eosio::name payer );

};
