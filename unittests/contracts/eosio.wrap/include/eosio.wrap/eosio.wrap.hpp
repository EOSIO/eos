#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/ignore.hpp>
#include <eosiolib/transaction.hpp>

namespace eosio {

   class [[eosio::contract("eosio.wrap")]] wrap : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void exec( ignore<name> executer, ignore<transaction> trx );

   };

} /// namespace eosio
