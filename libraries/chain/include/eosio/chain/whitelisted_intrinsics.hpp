
/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

   using whitelisted_intrinsics_type = shared_flat_multimap<uint64_t, shared_string>;

   // TODO: Improve performance by using std::string_view when we switch to C++17.

   bool is_intrinsic_whitelisted( const whitelisted_intrinsics_type& whitelisted_intrinsics, const std::string& name );

   void add_intrinsic_to_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics, const std::string& name );

   void remove_intrinsic_from_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics, const std::string& name );

} } // namespace eosio::chain
