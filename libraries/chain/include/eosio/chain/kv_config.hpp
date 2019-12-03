#pragma once

#include <fc/reflect/reflect.hpp>
#include <cstdint>

namespace eosio { namespace chain {

   /**
    * @brief limits for a kv database.
    *
    * Each database (ram or disk, currently) has its own limits for these parameters.
    * These limits apply when adding or modifying elements.  The limits may be reduced
    * below existing database entries.
    */
   struct kv_database_config {
      std::uint32_t max_key_size;   ///< the maximum size in bytes of a key
      std::uint32_t max_value_size; ///< the maximum size in bytes of a value
   };

   struct kv_config {
      kv_database_config kvram;
      kv_database_config kvdisk;
   };

}}

FC_REFLECT(eosio::chain::kv_database_config, (max_key_size)(max_value_size))
FC_REFLECT(eosio::chain::kv_config, (kvram)(kvdisk))
