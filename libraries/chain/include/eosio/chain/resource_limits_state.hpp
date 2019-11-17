#pragma once
#include <cstdint>

namespace eosio { namespace chain { namespace resource_limits {
   enum class resource_limits_state : uint8_t {
      final     = 0,
      pending   = 1,
      transient = 2
   };
}}} // namespace eosio::chain::resource_limits
