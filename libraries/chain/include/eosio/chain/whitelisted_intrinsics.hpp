#pragma once

#include <eosio/chain/types.hpp>

#include <string_view>

namespace eosio { namespace chain {

   using whitelisted_intrinsics_type = shared_flat_multimap<uint64_t, shared_string>;

   // TODO: Improve performance by using std::string_view when we switch to C++17.

   bool is_intrinsic_whitelisted( const whitelisted_intrinsics_type& whitelisted_intrinsics, const std::string_view name );

   void add_intrinsic_to_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics, const std::string_view name );

   template <typename...Rest>
   void add_intrinsics_to_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics, const std::string_view name, Rest&&... rest) {
      add_intrinsic_to_whitelist(whitelisted_intrinsics, name);
      if constexpr (sizeof...(Rest) != 0)
         add_intrinsics_to_whitelist(whitelisted_intrinsics, std::forward<Rest>(rest)...);
   }

   void remove_intrinsic_from_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics, const std::string_view name );

   void reset_intrinsic_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics,
                                   const std::set<std::string>& s );

   std::set<std::string> convert_intrinsic_whitelist_to_set( const whitelisted_intrinsics_type& whitelisted_intrinsics );

} } // namespace eosio::chain
