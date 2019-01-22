/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/contract_types.hpp>

namespace eosio { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_eosio_newaccount(apply_context&);
   void apply_eosio_updateauth(apply_context&);
   void apply_eosio_deleteauth(apply_context&);
   void apply_eosio_linkauth(apply_context&);
   void apply_eosio_unlinkauth(apply_context&);

   void apply_eosio_providebw(apply_context&);

   void apply_eosio_requestbw(apply_context&);

   void apply_eosio_newdomain(apply_context&);
   void apply_eosio_passdomain(apply_context&);
   void apply_eosio_linkdomain(apply_context&);
   void apply_eosio_unlinkdomain(apply_context&);
   void apply_eosio_newusername(apply_context&);

   /*
   void apply_eosio_postrecovery(apply_context&);
   void apply_eosio_passrecovery(apply_context&);
   void apply_eosio_vetorecovery(apply_context&);
   */

   void apply_eosio_setcode(apply_context&);
   void apply_eosio_setabi(apply_context&);

   void apply_eosio_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace eosio::chain
