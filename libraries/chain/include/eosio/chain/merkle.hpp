#pragma once
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

   digest_type make_canonical_left(const digest_type& val);
   digest_type make_canonical_right(const digest_type& val);

   bool is_canonical_left(const digest_type& val);
   bool is_canonical_right(const digest_type& val);


   inline auto make_canonical_pair(const digest_type& l, const digest_type& r) {
      return make_pair(make_canonical_left(l), make_canonical_right(r));
   };

   /**
    *  Calculates the merkle root of a set of digests, if ids is odd it will duplicate the last id.
    */
   digest_type merkle( deque<digest_type> ids );

} } /// eosio::chain
