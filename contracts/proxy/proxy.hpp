/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eoslib/eos.hpp>
#include <eoslib/db.hpp>

namespace proxy {
   struct PACKED( Config ) {
      Config( account_name o = account_name() ):owner(o){}
      const uint64_t     key = N(config);
      account_name        owner;
   };

   using Configs = Table<N(proxy),N(proxy),N(configs),Config,uint64_t>;

} /// namespace proxy