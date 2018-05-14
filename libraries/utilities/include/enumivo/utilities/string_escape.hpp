/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <string>

namespace eosio { namespace utilities {

  std::string escape_string_for_c_source_code(const std::string& input);

} } // end namespace eosio::utilities
