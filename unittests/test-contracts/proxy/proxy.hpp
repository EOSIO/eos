#pragma once

#include <apifiny/apifiny.hpp>
#include <apifiny/singleton.hpp>
#include <apifiny/asset.hpp>

// Extacted from apifiny.token contract:
namespace apifiny {
   class [[apifiny::contract("apifiny.token")]] token : public apifiny::contract {
   public:
      using apifiny::contract::contract;

      [[apifiny::action]]
      void transfer( apifiny::name        from,
                     apifiny::name        to,
                     apifiny::asset       quantity,
                     const std::string& memo );
      using transfer_action = apifiny::action_wrapper<"transfer"_n, &token::transfer>;
   };
}

// This contract:
class [[apifiny::contract]] proxy : public apifiny::contract {
public:
   proxy( apifiny::name self, apifiny::name first_receiver, apifiny::datastream<const char*> ds );

   [[apifiny::action]]
   void setowner( apifiny::name owner, uint32_t delay );

   [[apifiny::on_notify("apifiny.token::transfer")]]
   void on_transfer( apifiny::name        from,
                     apifiny::name        to,
                     apifiny::asset       quantity,
                     const std::string& memo );

   [[apifiny::on_notify("apifiny::onerror")]]
   void on_error( uint128_t sender_id, apifiny::ignore<std::vector<char>> sent_trx );

   struct [[apifiny::table]] config {
      apifiny::name owner;
      uint32_t    delay   = 0;
      uint32_t    next_id = 0;

      EOSLIB_SERIALIZE( config, (owner)(delay)(next_id) )
   };

   using config_singleton = apifiny::singleton< "config"_n,  config >;

protected:
   config_singleton _config;
};
