#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::require_auth( account_name account ) const {
      context.require_authorization( account );
   }

   bool interface::has_auth( account_name account ) const {
      return context.has_authorization( account );
   }

   void interface::require_auth2( account_name account,
                       permission_name permission ) const {
      context.require_authorization( account, permission );
   }

   void interface::require_recipient( account_name recipient ) {
      context.require_recipient( recipient );
   }

   bool interface::is_account( account_name account ) const {
      return context.is_account( account );
   }
}}} // ns eosio::chain::webassembly
