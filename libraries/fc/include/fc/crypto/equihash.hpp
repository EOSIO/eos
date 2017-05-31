#pragma once
#include <fc/crypto/sha256.hpp>
#include <fc/vector.hpp>

namespace fc { namespace equihash {

   struct proof
   {
      uint32_t n;
      uint32_t k;
      sha256   seed;
      std::vector< uint32_t > inputs;

      bool is_valid() const;

      static proof hash( uint32_t n, uint32_t k, sha256 seed );
   };

} } // fc

FC_REFLECT( fc::equihash::proof, (n)(k)(seed)(inputs) )
