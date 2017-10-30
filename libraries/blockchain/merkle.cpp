#include <eosio/blockchain/merkle.hpp>
#include <fc/io/raw.hpp>

namespace eosio { namespace blockchain {

digest_type merkle(vector<digest_type> ids) {

   while (ids.size() > 1) {
      if (ids.size() % 2)
         ids.push_back(ids.back());
      for (int i = 0; i < ids.size() / 2; i++)
         ids[i] = digest_type::hash(std::make_pair(ids[2*i], ids[(2*i)+1]));
      ids.resize(ids.size() / 2);
   }

   return ids.front();
}

} } // eosio::blockchain
