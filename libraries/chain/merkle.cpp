#include <eosio/chain/merkle.hpp>
#include <fc/io/raw.hpp>

namespace eosio { namespace chain {

digest_type merkle(vector<digest_type> ids) {
   if( 0 == ids.size() ) { return digest_type(); }

   while( ids.size() > 1 ) {
      if( ids.size() % 2 )
         ids.push_back(ids.back());

      for (int i = 0; i < ids.size() / 2; i++)
         ids[i] = digest_type::hash( make_pair(ids[2*i], ids[(2*i)+1]) );

      ids.resize(ids.size() / 2);
   }

   return ids.front();
}

} } // eosio::chain
