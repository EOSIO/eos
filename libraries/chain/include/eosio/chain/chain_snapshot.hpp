/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

struct chain_snapshot_header {
   /**
    * Version history
    *   1: initial version
    *   2: Updated chain snapshot for v1.8.0 initial protocol features release:
    *         - Incompatible with version 1.
    *         - Adds new indices for: protocol_state_object and account_ram_correction_object
    */

   static constexpr uint32_t minimum_compatible_version = 2;
   static constexpr uint32_t current_version = 2;

   uint32_t version = current_version;

   void validate() const {
      auto min = minimum_compatible_version;
      auto max = current_version;
      EOS_ASSERT(version >= min && version <= max,
              snapshot_validation_exception,
              "Unsupported version of chain snapshot: ${version}. Supported version must be between ${min} and ${max} inclusive.",
              ("version",version)("min",min)("max",max));
   }
};

} }

FC_REFLECT(eosio::chain::chain_snapshot_header,(version))
