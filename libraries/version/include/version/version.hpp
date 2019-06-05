/**
 *  @file version.hpp
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <string> // std::string_view

namespace eosio { namespace version {

   ///< Grab the version of the client in the form of: `v1.8.0-rc1`
   const std::string_view& version_client();
      
   ///< Grab the version of the client in the form of: `v1.8.0-rc1-7de458254[-dirty]`
   const std::string_view& version_full();
      
} }
