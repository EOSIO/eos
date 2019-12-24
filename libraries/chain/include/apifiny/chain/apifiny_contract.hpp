#pragma once

#include <apifiny/chain/types.hpp>
#include <apifiny/chain/contract_types.hpp>

namespace apifiny { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_apifiny_newaccount(apply_context&);
   void apply_apifiny_updateauth(apply_context&);
   void apply_apifiny_deleteauth(apply_context&);
   void apply_apifiny_linkauth(apply_context&);
   void apply_apifiny_unlinkauth(apply_context&);

   /*
   void apply_apifiny_postrecovery(apply_context&);
   void apply_apifiny_passrecovery(apply_context&);
   void apply_apifiny_vetorecovery(apply_context&);
   */

   void apply_apifiny_setcode(apply_context&);
   void apply_apifiny_setabi(apply_context&);

   void apply_apifiny_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace apifiny::chain
