#pragma once

#include <regex>
#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/abi_def.hpp>

namespace eosio::trace_api::configuration_utils {
   using namespace eosio;

   /**
    * Given a path (absolute or relative) to a file that contains a JSON-encoded ABI, return the parsed ABI
    *
    * @param file_name - a path to the ABI
    * @param data_dir - the base path for relative file_name values
    * @return the ABI implied
    * @throws json_parse_exception if the JSON is malformed
    */
   chain::abi_def abi_def_from_file(const std::string& file_name, const fc::path& data_dir )
   {
      fc::variant abi_variant;
      auto abi_path = fc::path(file_name);
      if (abi_path.is_relative()) {
         abi_path = data_dir / abi_path;
      }

      EOS_ASSERT(fc::exists(abi_path) && !fc::is_directory(abi_path), chain::plugin_config_exception, "${path} does not exist or is not a file", ("path", abi_path.generic_string()));
      try {
         abi_variant = fc::json::from_file(abi_path);
      } EOS_RETHROW_EXCEPTIONS(chain::json_parse_exception, "Fail to parse JSON from file: ${file}", ("file", abi_path.generic_string()));

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