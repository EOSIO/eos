#include <eosio/tester_module/tester.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/version/version.hpp>

#include <iostream>

namespace eosio { namespace tester_module {
   extern "C" void assert_version(uint64_t major, uint64_t minor, uint64_t patch, uint8_t policy) {
      using namespace eosio::tester_module;
      EOS_ASSERT(eosio::version::does_version_satisfy_policy(major, minor, patch, static_cast<enum policy>(policy)),
            chain::version_exception, "Version policy is not satisfied");
   }
}}
