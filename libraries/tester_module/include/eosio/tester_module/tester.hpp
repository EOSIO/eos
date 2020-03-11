#pragma once

#include <cstdint>

namespace eosio { namespace tester_module {
   enum class policy : uint8_t {
      strict,
      major,
      minor,
      patch
   };
}}

extern "C" {
   void check_version(uint64_t major, uint64_t minor, uint64_t patch, uint8_t policy);
}
