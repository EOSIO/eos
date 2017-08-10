#pragma once

#include <eos/chain/types.hpp>
#include <eos/types/generated.hpp>

#include <eos/utilities/parallel_markers.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/range/algorithm/find.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

namespace eos { namespace chain {

namespace detail {
using MetaPermission = static_variant<types::KeyPermissionWeight, types::AccountPermissionWeight>;

struct GetWeightVisitor {
   using result_type = UInt32;

   template<typename Permission>
   UInt32 operator()(const Permission& permission) { return permission.weight; }
};

// Orders permissions descending by weight, and breaks ties with Key permissions being less than Account permissions
struct MetaPermissionComparator {
   bool operator()(const MetaPermission& a, const MetaPermission& b) const {
      GetWeightVisitor scale;
      if (a.visit(scale) > b.visit(scale)) return true;
      return a.contains<types::KeyPermissionWeight>() && !b.contains<types::KeyPermissionWeight>();
   }
};

using MetaPermissionSet = boost::container::flat_multiset<MetaPermission, MetaPermissionComparator>;
}

/**
 * @brief This class determines whether a set of signing keys are sufficient to satisfy an authority or not
 *
 * To determine whether an authority is satisfied or not, we first determine which keys have approved of a message, and
 * then determine whether that list of keys is sufficient to satisfy the authority. This class takes a list of keys and
 * provides the @ref satisfied method to determine whether that list of keys satisfies a provided authority.
 *
 * @tparam F A callable which takes a single argument of type @ref AccountPermission and returns the corresponding
 * authority
 */
template<typename F>
class AuthorityChecker {
   F PermissionToAuthority;
   UInt16 recursionDepthLimit;
   vector<public_key_type> signingKeys;
   vector<bool> usedKeys;

   struct WeightTallyVisitor {
      using result_type = UInt32;

      AuthorityChecker& checker;
      UInt16 recursionDepth;
      UInt32 totalWeight = 0;

      WeightTallyVisitor(AuthorityChecker& checker, UInt16 recursionDepth)
         : checker(checker), recursionDepth(recursionDepth) {}

      UInt32 operator()(const types::KeyPermissionWeight& permission) {
         auto itr = boost::find(checker.signingKeys, permission.key);
         if (itr != checker.signingKeys.end()) {
            checker.usedKeys[itr - checker.signingKeys.begin()] = true;
            totalWeight += permission.weight;
         }
         return totalWeight;
      }
      UInt32 operator()(const types::AccountPermissionWeight& permission) {
         if (recursionDepth < checker.recursionDepthLimit
             && checker.satisfied(permission.permission, recursionDepth + 1))
            totalWeight += permission.weight;
         return totalWeight;
      }
   };

public:
   AuthorityChecker(F PermissionToAuthority, UInt16 recursionDepthLimit, const flat_set<public_key_type>& signingKeys)
      : PermissionToAuthority(PermissionToAuthority),
        recursionDepthLimit(recursionDepthLimit),
        signingKeys(signingKeys.begin(), signingKeys.end()),
        usedKeys(signingKeys.size(), false)
   {}

   bool satisfied(const types::AccountPermission& permission, UInt16 depth = 0) {
      return satisfied(PermissionToAuthority(permission), depth);
   }
   template<typename AuthorityType>
   bool satisfied(const AuthorityType& authority, UInt16 depth = 0) {
      // This check is redundant, since WeightTallyVisitor did it too, but I'll leave it here for future-proofing
      if (depth > recursionDepthLimit)
         return false;

      // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
      auto KeyReverter = fc::make_scoped_exit([this, keys = usedKeys] () mutable {
         usedKeys = keys;
      });

      // Sort key permissions and account permissions together into a single set of MetaPermissions
      detail::MetaPermissionSet permissions;
      permissions.insert(authority.keys.begin(), authority.keys.end());
      permissions.insert(authority.accounts.begin(), authority.accounts.end());

      // Check all permissions, from highest weight to lowest, seeing if signingKeys satisfies them or not
      WeightTallyVisitor visitor(*this, depth);
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
      auto range = utilities::FilterDataByMarker(signingKeys, usedKeys, true);
      return {range.begin(), range.end()};
   }
   flat_set<public_key_type> unused_keys() const {
      auto range = utilities::FilterDataByMarker(signingKeys, usedKeys, false);
      return {range.begin(), range.end()};
   }
};

template<typename F>
AuthorityChecker<F> MakeAuthorityChecker(F&& pta, UInt16 recursionDepthLimit,
                                         const flat_set<public_key_type>& signingKeys) {
   return AuthorityChecker<F>(std::forward<F>(pta), recursionDepthLimit, signingKeys);
}

}} // namespace eos::chain
