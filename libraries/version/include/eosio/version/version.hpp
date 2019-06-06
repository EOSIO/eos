/**
 *  @file version.hpp
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <string> // std::string

namespace eosio { namespace version {

   ///< Grab the version of the client in the form of: `v1.8.0-rc1`
   const std::string& version_client();
      
   ///< Grab the version of the client in the form of: `v1.8.0-rc1-7de458254[-dirty]`
   const std::string& version_full();
      
} }
