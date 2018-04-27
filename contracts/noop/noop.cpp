/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>

namespace eosio {

   class noop: public contract {
      public:
         noop( account_name self ): contract( self ) { }
         void anyaction( account_name from,
                         const std::string& /*type*/,
                         const std::string& /*data*/ )
         {
            require_auth( from );
         }
   };

   EOSIO_ABI( noop, ( anyaction ) )

} /// eosio     
