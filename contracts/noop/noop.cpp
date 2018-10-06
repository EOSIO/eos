/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */

#include <arisenlib/eosio.hpp>

namespace arisen {

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
