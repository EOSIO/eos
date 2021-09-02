#pragma once

#include <fc/filesystem.hpp>
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
   chain::abi_def abi_def_from_file(const std::string& file_name, const fc::path& data_dir );

   /**
    * Given a string in the form <key>=<value> where key cannot contain an `=` character and value can contain anything
    * return a pair of the two independent strings
    *
    * @param input
    * @return
    */
   std::pair<std::string, std::string> parse_kv_pairs( const std::string& input );

}