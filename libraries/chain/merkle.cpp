#include <eosio/chain/merkle.hpp>
#include <fc/io/raw.hpp>

namespace eosio { namespace chain {

/**
 * in order to keep proofs concise, before hashing we set the first bit
 * of the previous hashes to 0 or 1 to indicate the side it is on
 *
 * this relieves our proofs from having to indicate left vs right contactenation
 * as the node values will imply it
 */

digest_type make_canonical_left(const digest_type& val) {
   digest_type canonical_l = val;
   canonical_l._hash[0] &= 0xFFFFFFFFFFFFFF7FULL;
   return canonical_l;
}

digest_type make_canonical_right(const digest_type& val) {
   digest_type canonical_r = val;
   canonical_r._hash[0] |= 0x0000000000000080ULL;
   return canonical_r;
}

bool is_canonical_left(const digest_type& val) {
   return (val._hash[0] & 0x0000000000000080ULL) == 0;
}

bool is_canonical_right(const digest_type& val) {
   return (val._hash[0] & 0x0000000000000080ULL) != 0;
}


digest_type merkle(vector<digest_type> ids) {
   size_t sz = ids.size();
   if(!sz) { return digest_type(); }
   size_t i;
   while( sz > 1 ) {
      if( sz % 2 )
         ids.push_back(ids.back());
      sz = ids.size();
      for (i = 0; i < sz / 2; i++) {
         ids[i] = digest_type::hash(make_canonical_pair(ids[2 * i], ids[(2 * i) + 1]));
      }
      ids.resize(sz/2);
      sz = ids.size();
   }
   return ids.front();
}

} // eosio::chain
