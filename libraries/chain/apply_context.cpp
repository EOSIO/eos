#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>

namespace eosio { namespace chain {
void apply_context::exec()
{
   auto native = mutable_controller.find_apply_handler( receiver, act.scope, act.name );
   if( native ) {
      (*native)(*this);
   } else {
      wlog( "WASM handler not yet implemented ${receiver}  ${scope}::${name}", ("receiver",receiver)("scope",act.scope)("name",act.name) );
   }
} /// exec()

void apply_context::require_authorization( const account_name& account ) {
  for( const auto& auth : act.authorization )
     if( auth.actor == account ) return;
  EOS_ASSERT( false, tx_missing_auth, "missing authority of ${account}", ("account",account));
}
void apply_context::require_authorization(const account_name& account, 
                                          const permission_name& permission) {
  for( const auto& auth : act.authorization )
     if( auth.actor == account ) {
        if( auth.permission == permission ) return;
     }
  EOS_ASSERT( false, tx_missing_auth, "missing authority of ${account}/${permission}", 
              ("account",account)("permission",permission) );
}

} } /// eosio::chain
