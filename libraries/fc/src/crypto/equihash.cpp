#include <equihash/pow.hpp>

#include <fc/crypto/equihash.hpp>

#define EQUIHASH_NONCE 2

namespace fc { namespace equihash {

   _POW::Seed sha_to_seed( sha256 seed )
   {
      _POW::Seed new_seed;

      // Seed is 128 bits. Half of sha256 to create seed. Should still have enough randomness
      new_seed.v[0] =  (unsigned int)  seed._hash[0];
      new_seed.v[0] ^= (unsigned int)  seed._hash[2];
      new_seed.v[1] =  (unsigned int)( seed._hash[0] >> 32 );
      new_seed.v[1] ^= (unsigned int)( seed._hash[2] >> 32 );
      new_seed.v[2] =  (unsigned int)  seed._hash[1];
      new_seed.v[2] ^= (unsigned int)  seed._hash[3];
      new_seed.v[3] =  (unsigned int)( seed._hash[1] >> 32 );
      new_seed.v[3] ^= (unsigned int)( seed._hash[3] >> 32 );

      return new_seed;
   }

   bool proof::is_valid() const
   {
      _POW::Proof test( n, k, sha_to_seed( seed ), EQUIHASH_NONCE, inputs );
      return test.Test();

   }

   proof proof::hash( uint32_t n, uint32_t k, sha256 seed )
   {
      auto hash = _POW::Equihash( n, k, sha_to_seed( seed ) );
      auto result = hash.FindProof( EQUIHASH_NONCE );

      proof p;
      p.n = n;
      p.k = k;
      p.seed = seed;
      p.inputs = result.inputs;

      return p;
   }

} } // fc::equihash
