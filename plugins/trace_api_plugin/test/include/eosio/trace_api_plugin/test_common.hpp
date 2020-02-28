#pragma once

namespace eosio::trace_api_plugin::test_common {
   /**
    * Utilities that make writing tests easier
    */

   fc::sha256 operator"" _h(const char* input, std::size_t) {
      return fc::sha256(input);
   }

   chain::name operator"" _n(const char* input, std::size_t) {
      return chain::name(input);
   }

   chain::asset operator"" _t(const char* input, std::size_t) {
      return chain::asset::from_string(input);
   }
}