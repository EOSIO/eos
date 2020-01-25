#pragma once

#include <eosio/eosio.hpp>
#include <vector>

class [[eosio::contract]] deferred_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action]]
   void defercall( eosio::name payer, uint64_t sender_id, eosio::name contract, uint64_t payload );

   [[eosio::action]]
   void delayedcall( eosio::name payer, uint64_t sender_id, eosio::name contract,
                     uint64_t payload, uint32_t delay_sec, bool replace_existing );

   [[eosio::action]]
   void deferfunc( uint64_t payload );
   using deferfunc_action = eosio::action_wrapper<"deferfunc"_n, &deferred_test::deferfunc>;

   [[eosio::action]]
   void inlinecall( eosio::name contract, eosio::name authorizer, uint64_t payload );

   [[eosio::action]]
   void fail();

   [[eosio::on_notify("eosio::onerror")]]
   void on_error( uint128_t sender_id, eosio::ignore<std::vector<char>> sent_trx );
};
