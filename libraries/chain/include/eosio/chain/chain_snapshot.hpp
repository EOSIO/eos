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
    *   3: Updated for v2.0.0 protocol features:
    *         - forwards compatible with version 2
    *         - WebAuthn keys
    *         - wtmsig block siganatures: the block header state changed to include producer authorities and additional signatures
    *         - removed genesis_state and added chain ID to global_property_object
    *   4: Updated for v3.0.0 protocol features:
    *         - forwards compatible with versions 2 and 3
    *         - KV database
    *         - DISK resource
    *         - Configurable wasm limits
    */

   static constexpr uint32_t minimum_compatible_version = 2;
   static constexpr uint32_t current_version = 4;

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
