#include "common.hpp"

#include <eosiolib/chain.h>

namespace identity {

   bool identity_base::is_trusted_by( eosio::account_name trusted, eosio::account_name by ) {
      trust_table t( _self, by );
      return t.find( trusted ) != t.end();
   }

   bool identity_base::is_trusted( eosio::account_name acnt ) {
      uint64_t active_producers[21];
      auto active_prod_size = get_active_producers( active_producers, sizeof(active_producers) );
      auto count = active_prod_size / sizeof(uint64_t);
      for( size_t i = 0; i < count; ++i ) {
         if( active_producers[i] == acnt )
            return true;
      }
      for( size_t i = 0; i < count; ++i ) {
         if( is_trusted_by( acnt, eosio::name{active_producers[i]} ) )
            return true;
      }
      return false;
   }

}
