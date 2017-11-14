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

} } /// eosio::chain
