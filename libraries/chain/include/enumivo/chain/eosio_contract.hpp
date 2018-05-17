/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */
#pragma once

#include <enumivo/chain/types.hpp>
#include <enumivo/chain/contract_types.hpp>

namespace enumivo { namespace chain { 

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_enumivo_newaccount(apply_context&);
   void apply_enumivo_updateauth(apply_context&);
   void apply_enumivo_deleteauth(apply_context&);
   void apply_enumivo_linkauth(apply_context&);
   void apply_enumivo_unlinkauth(apply_context&);

   /*
   void apply_enumivo_postrecovery(apply_context&);
   void apply_enumivo_passrecovery(apply_context&);
   void apply_enumivo_vetorecovery(apply_context&);
   */

   void apply_enumivo_setcode(apply_context&);
   void apply_enumivo_setabi(apply_context&);

   void apply_enumivo_canceldelay(apply_context&);
   ///@}  end action handlers
   
   abi_def enumivo_contract_abi(const abi_def& enumivo_system_abi);

} } /// namespace enumivo::chain
