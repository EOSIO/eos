/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

namespace eosio {

   CONTRACT testinline : public eosio::contract {
   public:
      using eosio::contract::contract;

      ACTION reqauth( name from ) {
         require_auth( from );
      }

      ACTION forward( name reqauth, name forward_code, name forward_auth ) {
         require_auth(reqauth);
         INLINE_ACTION_SENDER( testinline, reqauth)( forward_code, {forward_auth,"active"_n}, {forward_auth} );
      }
   };

} /// namespace eosio
