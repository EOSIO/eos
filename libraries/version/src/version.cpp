/**
 *  @file version.cpp
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <version.hpp>
#include <version_impl.hpp>

namespace eosio { namespace version {

   const std::string_view& version_client() {
      return static const std::string{_version_client()};
   }
      
   const std::string_view& version_full() {
      return static const std::string{_version_full()};
   }
      
} }
