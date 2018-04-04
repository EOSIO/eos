/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/apply_context.hpp>

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace contracts {

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_eosio_newaccount(apply_context&);
   void apply_eosio_updateauth(apply_context&);
   void apply_eosio_deleteauth(apply_context&);
   void apply_eosio_linkauth(apply_context&);
   void apply_eosio_unlinkauth(apply_context&);

   void apply_eosio_postrecovery(apply_context&);
   void apply_eosio_passrecovery(apply_context&);
   void apply_eosio_vetorecovery(apply_context&);

   void apply_eosio_setcode(apply_context&);
   void apply_eosio_setabi(apply_context&);

   void apply_eosio_onerror(apply_context&);

   void apply_eosio_canceldelay(apply_context&);
   ///@}  end action handlers

} } } /// namespace eosio::contracts
