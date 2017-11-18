#pragma once
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

   /**
    *  Calculates the merkle root of a set of digests, if ids is odd it will duplicate the last id.
    */
   digest_type merkle( vector<digest_type> ids );

} } /// eosio::chain
