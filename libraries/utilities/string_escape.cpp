/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/utilities/string_escape.hpp>
#include <sstream>

namespace eosio { namespace utilities {

  std::string escape_string_for_c_source_code(const std::string& input)
  {
    std::ostringstream escaped_string;
    escaped_string << "\"";
    for (unsigned i = 0; i < input.size(); ++i)
    {
      switch (input[i])
      {
      case '\a': 
        escaped_string << "\\a";
        break;
      case '\b': 
        escaped_string << "\\b";
        break;
      case '\t': 
        escaped_string << "\\t";
        break;
      case '\n': 
        escaped_string << "\\n";
        break;
      case '\v': 
        escaped_string << "\\v";
        break;
      case '\f': 
        escaped_string << "\\f";
        break;
      case '\r': 
        escaped_string << "\\r";
        break;
      case '\\': 
        escaped_string << "\\\\";
        break;
      case '\"': 
        escaped_string << "\\\"";
        break;
      default:
        escaped_string << input[i];
      }
    }
    escaped_string << "\"";
    return escaped_string.str();
  }

} } // end namespace eosio::utilities

