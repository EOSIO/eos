#pragma once

#include <regex>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/abi_def.hpp>

namespace eosio::trace_api_plugin::configuration_utils {
   using namespace eosio;

   /**
    * Give a string, parse a quantity of microseconds from it where the string is either
    *  - a number of seconds
    *  - a number with a suffix indicating Seconds, Minutes, Hours, Days, or Weeks
    *  - "-1" meaning maximum
    *
    * @param str - the input string
    * @return the indicated number of microseconds
    * @throws plugin_config_exception if the string is not in one of the above forms
    */
   fc::microseconds parse_microseconds(const std::string& str) {
      if (str == "-1") {
         return fc::microseconds::maximum();
      }

      std::regex r("^([0-9]+)([smhdw])?$");
      std::smatch m;
      std::regex_search(str, m, r);

      EOS_ASSERT(!m.empty(), chain::plugin_config_exception, "string is not a positive number with an optional suffix OR -1");
      int64_t number = std::stoll(m[1]);
      int64_t mult = 1'000'000;

      if (m.size() == 3 && m[2].length() != 0) {
         auto suffix = m[2].str();
         if (suffix == "m") {
            mult *= 60;
         } else if (suffix == "h") {
            mult *= 60 * 60;
         } else if (suffix == "d") {
            mult *= 60 * 60 * 24;
         } else if (suffix == "w") {
            mult *= 60 * 60 * 24 * 7;
         }
      }

      auto final = number * mult;
      EOS_ASSERT(final / mult == number, chain::plugin_config_exception, "specified time overflows, use \"-1\" to represent infinite time");

      return fc::microseconds(final);
   }

   /**
    * Give a string which is either a JSON-encoded ABI or a path (absolute) to a file that contains a
    * JSON-encoded ABI, return the parsed ABI
    *
    * @param file_or_str - a string that is either an ABI or path
    * @return the ABI implied
    * @throws json_parse_exception if the JSON is malformed
    */
   chain::abi_def abi_def_from_file_or_str(const std::string& file_or_str)
   {
      fc::variant abi_variant;
      std::regex r("^[ \t]*[\{\[\"]");
      if ( !std::regex_search(file_or_str, r) ) {
         EOS_ASSERT(fc::exists(file_or_str) && !fc::is_directory(file_or_str), chain::plugin_config_exception, "${path} does not exist or is not a file", ("path", file_or_str));
         try {
            abi_variant = fc::json::from_file(file_or_str);
         } EOS_RETHROW_EXCEPTIONS(chain::json_parse_exception, "Fail to parse JSON from file: ${file}", ("file", file_or_str));

      } else {
         try {
            abi_variant = fc::json::from_string(file_or_str);
         } EOS_RETHROW_EXCEPTIONS(chain::json_parse_exception, "Fail to parse JSON from string: ${string}", ("string", file_or_str));
      }

      chain::abi_def result;
      fc::from_variant(abi_variant, result);
      return result;
   }

   /**
    * Given a string in the form <key>=<value> where key cannot contain an `=` character and value can contain anything
    * return a pair of the two independent strings
    *
    * @param input
    * @return
    */
   std::pair<std::string, std::string> parse_kv_pairs( const std::string& input ) {
      EOS_ASSERT(!input.empty(), chain::plugin_config_exception, "Key-Value Pair is Empty");
      auto delim = input.find("=");
      EOS_ASSERT(delim != std::string::npos, chain::plugin_config_exception, "Missing \"=\"");
      EOS_ASSERT(delim != 0, chain::plugin_config_exception, "Missing Key");
      EOS_ASSERT(delim + 1 != input.size(), chain::plugin_config_exception, "Missing Value");
      return std::make_pair(input.substr(0, delim), input.substr(delim + 1));
   }

}