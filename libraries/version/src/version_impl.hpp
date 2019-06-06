/**
 *  @file version_impl.hpp
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <string> // std::string

namespace eosio { namespace version {

   ///< Helper function for `version_client()`
   const std::string& _version_client();

   ///< Helper function for `version_full()`
   const std::string& _version_full();
      
} }
