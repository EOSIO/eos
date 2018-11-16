#include <eosio.wrap/eosio.wrap.hpp>

namespace eosio {

void wrap::exec( ignore<name>, ignore<transaction> ) {
   require_auth( _self );

   name executer;
   _ds >> executer;

   require_auth( executer );

   send_deferred( (uint128_t(executer.value) << 64) | current_time(), executer.value, _ds.pos(), _ds.remaining() );
}

} /// namespace eosio

EOSIO_DISPATCH( eosio::wrap, (exec) )
