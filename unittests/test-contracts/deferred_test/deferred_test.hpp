#pragma once

#include <apifiny/apifiny.hpp>
#include <vector>

class [[apifiny::contract]] deferred_test : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action]]
   void defercall( apifiny::name payer, uint64_t sender_id, apifiny::name contract, uint64_t payload );

   [[apifiny::action]]
   void delayedcall( apifiny::name payer, uint64_t sender_id, apifiny::name contract,
                     uint64_t payload, uint32_t delay_sec, bool replace_existing );

   [[apifiny::action]]
   void deferfunc( uint64_t payload );
   using deferfunc_action = apifiny::action_wrapper<"deferfunc"_n, &deferred_test::deferfunc>;

   [[apifiny::action]]
   void inlinecall( apifiny::name contract, apifiny::name authorizer, uint64_t payload );

   [[apifiny::action]]
   void fail();

   [[apifiny::on_notify("apifiny::onerror")]]
   void on_error( uint128_t sender_id, apifiny::ignore<std::vector<char>> sent_trx );
};
