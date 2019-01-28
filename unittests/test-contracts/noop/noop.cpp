/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/eosio.hpp>

namespace eosio {

   CONTRACT noop: public contract {
   public:
      using contract::contract;

      // Ignore type in action
      ACTION anyaction( name from,
                        const eosio::ignore<std::string>& type,
                        const eosio::ignore<std::string>& data )
      {
         require_auth( from );
      }
   };

   EOSIO_DISPATCH( noop, ( anyaction ) )
    
} /// namespace eosio     
