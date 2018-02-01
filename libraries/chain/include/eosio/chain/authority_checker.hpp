/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>

#include <eosio/utilities/parallel_markers.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/range/algorithm/find.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

namespace eosio { namespace chain {

namespace detail {

   using meta_permission = static_variant<key_weight, permission_level_weight>;

   struct get_weight_visitor {
      using result_type = uint32_t;

      template<typename Permission>
      uint32_t operator()(const Permission& permission) { return permission.weight; }
   };

   // Orders permissions descending by weight, and breaks ties with Key permissions being less than Account permissions
   struct meta_permission_comparator {
      bool operator()(const meta_permission& a, const meta_permission& b) const {
         get_weight_visitor scale;
         if (a.visit(scale) > b.visit(scale)) return true;
         return a.contains<key_weight>() && !b.contains<permission_level_weight>();
      }
   };

   using meta_permission_set = boost::container::flat_multiset<meta_permission, meta_permission_comparator>;

} /// namespace detail

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
   template<typename PermissionToAuthorityFunc>
   class authority_checker {
      private:
         PermissionToAuthorityFunc  permission_to_authority;
         uint16_t                   recursion_depth_limit;
         vector<public_key_type>    signing_keys;
         flat_set<account_name>     _provided_auths; /// accounts which have authorized the transaction at owner level
         vector<bool>               _used_keys;

         struct weight_tally_visitor {
            using result_type = uint32_t;

            authority_checker& checker;
            uint16_t recursion_depth;
            uint32_t total_weight = 0;

            weight_tally_visitor(authority_checker& checker, uint16_t recursion_depth)
               : checker(checker), recursion_depth(recursion_depth) {}

            uint32_t operator()(const key_weight& permission) {
               auto itr = boost::find(checker.signing_keys, permission.key);
               if (itr != checker.signing_keys.end()) {
                  checker._used_keys[itr - checker.signing_keys.begin()] = true;
                  total_weight += permission.weight;
               }
               return total_weight;
            }
            uint32_t operator()(const permission_level_weight& permission) {
               if (recursion_depth < checker.recursion_depth_limit) {
                  if( checker.has_permission( permission.permission.actor ) )
                     total_weight += permission.weight;
                  else if( checker.satisfied(permission.permission, recursion_depth + 1) )
                     total_weight += permission.weight;
               }
               return total_weight;
            }
         };

         bool has_permission( account_name n )const {
            return _provided_auths.find(n) != _provided_auths.end();
         }

      public:
         authority_checker( PermissionToAuthorityFunc permission_to_authority, 
                            uint16_t recursion_depth_limit, const flat_set<public_key_type>& signing_keys,
                            flat_set<account_name> provided_auths = flat_set<account_name>() )
            : permission_to_authority(permission_to_authority),
              recursion_depth_limit(recursion_depth_limit),
              signing_keys(signing_keys.begin(), signing_keys.end()),
              _provided_auths(provided_auths.begin(), provided_auths.end()),
              _used_keys(signing_keys.size(), false)
         {}

         bool satisfied(const permission_level& permission, uint16_t depth = 0) {
            return has_permission( permission.actor ) ||
                   satisfied(permission_to_authority(permission), depth);
         }

         template<typename AuthorityType>
         bool satisfied(const AuthorityType& authority, uint16_t depth = 0) {
            // This check is redundant, since weight_tally_visitor did it too, but I'll leave it here for future-proofing
            if (depth > recursion_depth_limit)
               return false;

            // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
            auto KeyReverter = fc::make_scoped_exit([this, keys = _used_keys] () mutable {
               _used_keys = keys;
            });

            // Sort key permissions and account permissions together into a single set of meta_permissions
            detail::meta_permission_set permissions;

            permissions.insert(authority.keys.begin(), authority.keys.end());
            permissions.insert(authority.accounts.begin(), authority.accounts.end());

            // Check all permissions, from highest weight to lowest, seeing if signing_keys satisfies them or not
            weight_tally_visitor visitor(*this, depth);
            for (const auto& permission : permissions)
               // If we've got enough weight, to satisfy the authority, return!
               if (permission.visit(visitor) >= authority.threshold) {
                  KeyReverter.cancel();
                  return true;
               }
            return false;
         }

         bool all_keys_used() const { return boost::algorithm::all_of_equal(_used_keys, true); }

         flat_set<public_key_type> used_keys() const {
            auto range = utilities::filter_data_by_marker(signing_keys, _used_keys, true);
            return {range.begin(), range.end()};
         }
         flat_set<public_key_type> unused_keys() const {
            auto range = utilities::filter_data_by_marker(signing_keys, _used_keys, false);
            return {range.begin(), range.end()};
         }
   }; /// authority_checker

   template<typename PermissionToAuthorityFunc>
   auto make_auth_checker(PermissionToAuthorityFunc&& pta, 
                          uint16_t recursion_depth_limit, 
                          const flat_set<public_key_type>& signing_keys,
                          const flat_set<account_name>& accounts = flat_set<account_name>() ) {
      return authority_checker<PermissionToAuthorityFunc>(std::forward<PermissionToAuthorityFunc>(pta), recursion_depth_limit, signing_keys, accounts);
   }

} } // namespace eosio::chain
