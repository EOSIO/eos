#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::require_auth( account_name account ) {
      ctx.require_authorization( account );
   }

   bool interface::has_auth( account_name account ) {
      return ctx.has_authorization( account );
   }

   void interface::require_auth2( account_name account,
                       permission_name permission ) {
      ctx.require_authorization( account, permission );
   }

   void interface::require_recipient( account_name recipient ) {
      ctx.require_recipient( recipient );
   }

   bool interface::is_account( account_name account ) {
      return ctx.is_account( account );
   }
}}} // ns eosio::chain::webassembly
