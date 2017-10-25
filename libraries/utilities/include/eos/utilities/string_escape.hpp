/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <string>

namespace eos { namespace utilities {

  std::string escape_string_for_c_source_code(const std::string& input);

} } // end namespace eos::utilities
