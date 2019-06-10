/**
 *  @copyright defined in eosio.cdt/LICENSE.txt
 */

#pragma once

#include <eosio/eosio.hpp>
#include <eosio/ignore.hpp>
#include <eosio/transaction.hpp>

namespace eosio {
   /**
    * @defgroup eosiowrap eosio.wrap
    * @ingroup eosiocontracts
    * eosio.wrap contract simplifies Block Producer superuser actions by making them more readable and easier to audit.

    * @details It does not grant block producers any additional powers that do not already exist within the 
    * system. Currently, 15/21 block producers can already change an account's keys or modify an 
    * account's contract at the request of ECAF or an account's owner. However, the current method 
    * is opaque and leaves undesirable side effects on specific system accounts. 
    * eosio.wrap allows for a cleaner method of implementing these important governance actions.
    * @{
    */
   class [[eosio::contract("eosio.wrap")]] wrap : public contract {
      public:
         using contract::contract;

         /**
          * Execute action.
          * 
          * @details Execute a transaction while bypassing regular authorization checks.
          * 
          * @param executer - account executing the transaction,
          * @param trx - the transaction to be executed.
          * 
          * @pre Requires authorization of eosio.wrap which needs to be a privileged account.
          * 
          * @post Deferred transaction RAM usage is billed to 'executer'
          */
         [[eosio::action]]
         void exec( ignore<name> executer, ignore<transaction> trx );

         using exec_action = eosio::action_wrapper<"exec"_n, &wrap::exec>;
   };
   /** @}*/ // end of @defgroup eosiowrap eosio.wrap
} /// namespace eosio
