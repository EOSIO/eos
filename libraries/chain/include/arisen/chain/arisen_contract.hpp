/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */
#pragma once

#include <arisen/chain/types.hpp>
#include <arisen/chain/contract_types.hpp>

namespace arisen { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_arisen_newaccount(apply_context&);
   void apply_arisen_updateauth(apply_context&);
   void apply_arisen_deleteauth(apply_context&);
   void apply_arisen_linkauth(apply_context&);
   void apply_arisen_unlinkauth(apply_context&);

   /*
   void apply_arisen_postrecovery(apply_context&);
   void apply_arisen_passrecovery(apply_context&);
   void apply_arisen_vetorecovery(apply_context&);
   */

   void apply_arisen_setcode(apply_context&);
   void apply_arisen_setabi(apply_context&);

   void apply_arisen_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace arisen::chain
