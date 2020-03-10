#pragma once

#include <regex>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/abi_def.hpp>

namespace eosio::trace_api::configuration_utils {
   using namespace eosio;

   /**
    * Give a string which is either a JSON-encoded ABI or a path (absolute) to a file that contains a
    * JSON-encoded ABI, return the parsed ABI
    *
    * @param file_or_str - a string that is either an ABI or path
    * @return the ABI implied
    * @throws json_parse_exception if the JSON is malformed
    */
   chain::abi_def abi_def_from_file_or_str(const std::string& file_or_str, const fc::path& data_dir )
   {
      fc::variant abi_variant;
      std::regex r("^[ \t]*[\{\[\"]");
      if ( !std::regex_search(file_or_str, r) ) {
         auto abi_path = fc::path(file_or_str);
         if (abi_path.is_relative()) {
            abi_path = data_dir / abi_path;
         }

         EOS_ASSERT(fc::exists(abi_path) && !fc::is_directory(abi_path), chain::plugin_config_exception, "${path} does not exist or is not a file", ("path", file_or_str));
         try {
            abi_variant = fc::json::from_file(abi_path);
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