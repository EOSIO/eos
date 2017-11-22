/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include <eoslib/db.hpp>

namespace proxy {
   
   //@abi action
   struct PACKED( set_owner ) {
      account_name owner;	
   };

   //@abi table
   struct PACKED( config ) {
      config( account_name o = account_name() ):owner(o){}
      const uint64_t     key = N(config);
      account_name        owner;
   };

   using configs = eosio::table<N(proxy),N(proxy),N(configs),config,uint64_t>;

} /// namespace proxy
