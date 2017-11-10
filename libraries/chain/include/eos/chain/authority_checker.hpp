/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/types.hpp>
#include <eos/types/generated.hpp>

#include <eos/utilities/parallel_markers.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/range/algorithm/find.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

namespace eosio { namespace chain {

namespace detail {
using meta_permission = static_variant<types::key_permission_weight, types::account_permission_weight>;

struct get_weight_visitor {
   using result_type = uint32;

   template<typename Permission>
   uint32 operator()(const Permission& permission) { return permission.weight; }
};

// Orders permissions descending by weight, and breaks ties with Key permissions being less than Account permissions
struct meta_permission_comparator {
   bool operator()(const meta_permission& a, const meta_permission& b) const {
      get_weight_visitor scale;
      if (a.visit(scale) > b.visit(scale)) return true;
      return a.contains<types::key_permission_weight>() && !b.contains<types::key_permission_weight>();
   }
};

using meta_permission_set = boost::container::flat_multiset<meta_permission, meta_permission_comparator>;
}

/**
 * @brief This class determines whether a set of signing keys are sufficient to satisfy an authority or not
 *
 * To determine whether an authority is satisfied or not, we first determine which keys have approved of a message, and
 * then determine whether that list of keys is sufficient to satisfy the authority. This class takes a list of keys and
 * provides the @ref satisfied method to determine whether that list of keys satisfies a provided authority.
 *
 * @tparam F A callable which takes a single argument of type @ref account_permission and returns the corresponding
 * authority
 */
template<typename F>
class authority_checker {
   F PermissionToAuthority;
   uint16 recursionDepthLimit;
   vector<public_key_type> signingKeys;
   vector<bool> usedKeys;

   struct weight_tally_visitor {
      using result_type = uint32;

      authority_checker& checker;
      uint16 recursionDepth;
      uint32 totalWeight = 0;

      weight_tally_visitor(authority_checker& checker, uint16 recursionDepth)
         : checker(checker), recursionDepth(recursionDepth) {}

      uint32 operator()(const types::key_permission_weight& permission) {
         auto itr = boost::find(checker.signingKeys, permission.key);
         if (itr != checker.signingKeys.end()) {
            checker.usedKeys[itr - checker.signingKeys.begin()] = true;
            totalWeight += permission.weight;
         }
         return totalWeight;
      }
      uint32 operator()(const types::account_permission_weight& permission) {
         if (recursionDepth < checker.recursionDepthLimit
             && checker.satisfied(permission.permission, recursionDepth + 1))
            totalWeight += permission.weight;
         return totalWeight;
      }
   };

public:
   authority_checker(F PermissionToAuthority, uint16 recursionDepthLimit, const flat_set<public_key_type>& signingKeys)
      : PermissionToAuthority(PermissionToAuthority),
        recursionDepthLimit(recursionDepthLimit),
        signingKeys(signingKeys.begin(), signingKeys.end()),
        usedKeys(signingKeys.size(), false)
   {}

   bool satisfied(const types::account_permission& permission, uint16 depth = 0) {
      return satisfied(PermissionToAuthority(permission), depth);
   }
   template<typename AuthorityType>
   bool satisfied(const AuthorityType& authority, uint16 depth = 0) {
      // This check is redundant, since weight_tally_visitor did it too, but I'll leave it here for future-proofing
      if (depth > recursionDepthLimit)
         return false;

      // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
      auto KeyReverter = fc::make_scoped_exit([this, keys = usedKeys] () mutable {
         usedKeys = keys;
      });

      // Sort key permissions and account permissions together into a single set of MetaPermissions
      detail::meta_permission_set permissions;
      permissions.insert(authority.keys.begin(), authority.keys.end());
      permissions.insert(authority.accounts.begin(), authority.accounts.end());

      // Check all permissions, from highest weight to lowest, seeing if signingKeys satisfies them or not
      weight_tally_visitor visitor(*this, depth);
      for (const auto& permission : permissions)
         // If we've got enough weight, to satisfy the authority, return!
         if (permission.visit(visitor) >= authority.threshold) {
            KeyReverter.cancel();
            return true;
         }
      return false;
   }

   bool all_keys_used() const { return boost::algorithm::all_of_equal(usedKeys, true); }
   flat_set<public_key_type> used_keys() const {
      auto range = utilities::filter_data_by_marker(signingKeys, usedKeys, true);
      return {range.begin(), range.end()};
   }
   flat_set<public_key_type> unused_keys() const {
      auto range = utilities::filter_data_by_marker(signingKeys, usedKeys, false);
      return {range.begin(), range.end()};
   }
};

template<typename F>
authority_checker<F> make_authority_checker(F&& pta, uint16 recursionDepthLimit,
                                            const flat_set <public_key_type>& signingKeys) {
   return authority_checker<F>(std::forward<F>(pta), recursionDepthLimit, signingKeys);
}

}} // namespace eosio::chain
