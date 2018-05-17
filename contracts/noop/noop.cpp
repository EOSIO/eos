/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */

#include <enumivolib/enumivo.hpp>

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

   ENUMIVO_ABI( noop, ( anyaction ) )

} /// eosio     
