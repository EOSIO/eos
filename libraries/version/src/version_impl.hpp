/**
 *  @file version_impl.hpp
 *  @copyright defined in eos/LICENSE
 */

#pragma once

#include <string> // std::string_view

namespace eosio { namespace version {
      
   constexpr std::string_view version_major;
   constexpr std::string_view version_minor;
   constexpr std::string_view version_patch;
   constexpr std::string_view version_suffix;
   constexpr std::string_view version_hash;
   constexpr std::string_view version_dirty;

   ///< Helper function `version_client()`
   const std::string_view& _version_client();

   ///< Helper function `version_full()`
   const std::string_view& _version_full();
      
} }
