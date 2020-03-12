#pragma once

#include <string>

namespace eosio { namespace version {
   enum class policy : uint8_t {
      strict,  // version should match exactly
      major,   // version should be greater than or equal to major
      minor,   // version should be greater than or equal to minor
      patch    // version should be greater than or equal to patch
   };

   ///< Grab the basic version information of the client; example: `v1.8.0-rc1`
   const std::string& version_client();

   ///< Grab the full version information of the client; example:  `v1.8.0-rc1-7de458254[-dirty]`
   const std::string& version_full();

   int64_t version_major();
   int64_t version_minor();
   int64_t version_patch();

   template <policy Policy>
   inline bool does_version_satisfy_policy(int64_t major, int64_t minor, int64_t patch) {
      if constexpr (Policy == policy::major) {
         return major <= version_major();
      } else if constexpr (Policy == policy::minor) {
         return major == version_major() && minor <= version_minor();
      } else if constexpr (Policy == policy::patch) {
         return major == version_major() && minor == version_minor() && patch <= version_patch();
      } else if constexpr (Policy == policy::strict) {
         return major == version_major() && minor == version_minor() && patch == version_patch();
      } else {
         return true;
      }
   }
} }
