/**
 *  @file
 *  @copyright defined in eos/LICENSE
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
   void apply_rem_newaccount(apply_context&);
   void apply_rem_updateauth(apply_context&);
   void apply_rem_deleteauth(apply_context&);
   void apply_rem_linkauth(apply_context&);
   void apply_rem_unlinkauth(apply_context&);

   /*
   void apply_rem_postrecovery(apply_context&);
   void apply_rem_passrecovery(apply_context&);
   void apply_rem_vetorecovery(apply_context&);
   */

   void apply_rem_setcode(apply_context&);
   void apply_rem_setabi(apply_context&);

   void apply_rem_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace eosio::chain
