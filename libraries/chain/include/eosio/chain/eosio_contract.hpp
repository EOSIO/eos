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
   void apply_cyber_newaccount(apply_context&);
   void apply_cyber_updateauth(apply_context&);
   void apply_cyber_deleteauth(apply_context&);
   void apply_cyber_linkauth(apply_context&);
   void apply_cyber_unlinkauth(apply_context&);

   void apply_cyber_providebw(apply_context&);
// TODO: requestbw
//   void apply_cyber_requestbw(apply_context&);
   void apply_cyber_provideram(apply_context&);

   void apply_cyber_domain_newdomain(apply_context&);
   void apply_cyber_domain_passdomain(apply_context&);
   void apply_cyber_domain_linkdomain(apply_context&);
   void apply_cyber_domain_unlinkdomain(apply_context&);
   void apply_cyber_domain_newusername(apply_context&);

   /*
   void apply_cyber_postrecovery(apply_context&);
   void apply_cyber_passrecovery(apply_context&);
   void apply_cyber_vetorecovery(apply_context&);
   */

   void apply_cyber_setcode(apply_context&);
   void apply_cyber_setabi(apply_context&);

   void apply_cyber_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace eosio::chain
