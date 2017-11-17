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

void apply_context::require_write_scope(const account_name& account)const {
   for( const auto& s : trx.write_scope )
      if( s == account ) return;

   if( trx.write_scope.size() == 1 && trx.write_scope.front() == config::eosio_all_scope )
      return;

   EOS_ASSERT( false, tx_missing_write_scope, "missing write scope ${account}", 
               ("account",account) );
}

void apply_context::require_read_scope(const account_name& account)const {
   for( const auto& s : trx.write_scope )
      if( s == account ) return;
   for( const auto& s : trx.read_scope )
      if( s == account ) return;

   if( trx.write_scope.size() == 1 && trx.write_scope.front() == config::eosio_all_scope )
      return;

   EOS_ASSERT( false, tx_missing_read_scope, "missing read scope ${account}", 
               ("account",account) );
}

} } /// eosio::chain
