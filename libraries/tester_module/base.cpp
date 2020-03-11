#include <eosio/tester_module/tester.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/version/version.hpp>

#include <iostream>

extern "C" {
   void check_version(uint64_t major, uint64_t minor, uint64_t patch, uint8_t policy) {
      using namespace eosio::tester_module;
      auto policy_used = static_cast<enum policy>(policy);
      if (policy_used == policy::major) {
         EOS_ASSERT(major <= eosio::version::version_major(), version_exception, "Major version doesn't match)
      }
   }
}
