#pragma once
#include <eosio/blockchain/types.hpp>

namespace eosio { namespace blockchain {

   /**
    *  Calculates the merkle root of a set of digests, if ids is odd it will duplicate the last id.
    */
   digest_type merkle( vector<digest_type> ids );

} } /// eosio::blockchain
