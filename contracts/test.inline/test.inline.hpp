#include <eosiolib/eosio.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/producer_schedule.hpp>

namespace eosio {

   class testinline : public contract {
      public:
         testinline( action_name self ):contract(self){}

         void reqauth( account_name from ) {
            require_auth( from );
         }

         void forward( action_name reqauth, account_name forward_code, account_name forward_auth ) {
            require_auth( reqauth );
            dispatch_inline( permission_level{forward_auth,N(active)}, forward_code, N(reqauth), &testinline::reqauth, { forward_auth } );
         }
   };

} /// namespace eosio
