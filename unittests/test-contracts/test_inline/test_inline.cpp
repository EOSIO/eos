/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "test_inline.hpp"

using namespace eosio;

namespace test_inline {

   void test_inline::require_authorization( name from ) {
      require_auth( from );
   }

   void test_inline::forward( name reqauth, name forward_code, name forward_auth ) {
      require_auth( reqauth );
      
      require_auth_action a( forward_code, {forward_auth, "active"_n} );
      a.send( forward_auth );
   }

} /// namespace test_inline
