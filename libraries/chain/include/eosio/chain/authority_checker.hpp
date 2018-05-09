/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/utilities/parallel_markers.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/range/algorithm/find.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

#include <functional>

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
         return a.contains<key_weight>() && b.contains<permission_level_weight>();
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
   template<typename PermissionToAuthorityFunc, typename PermissionVisitor>
   class authority_checker {
      private:
         PermissionToAuthorityFunc            permission_to_authority;
         PermissionVisitor                    permission_visitor;
         const std::function<void(uint32_t)>& checktime;
         vector<public_key_type>              signing_keys; // Making this a flat_set<public_key_type> causes runtime problems with utilities::filter_data_by_marker for some reason. TODO: Figure out why.
         flat_set<permission_level>           provided_permissions;
         vector<bool>                         _used_keys;
         fc::microseconds                     delay_threshold;
         uint16_t                             recursion_depth_limit;

      public:
         authority_checker( PermissionToAuthorityFunc            permission_to_authority,
                            PermissionVisitor                    permission_visitor,
                            uint16_t                             recursion_depth_limit,
                            const flat_set<public_key_type>&     signing_keys,
                            const flat_set<permission_level>&    provided_permissions,
                            fc::microseconds                     delay_threshold,
                            const std::function<void(uint32_t)>& checktime
                         )
         :permission_to_authority(permission_to_authority)
         ,permission_visitor(permission_visitor)
         ,checktime( checktime )
         ,signing_keys(signing_keys.begin(), signing_keys.end())
         ,provided_permissions(provided_permissions)
         ,_used_keys(signing_keys.size(), false)
         ,delay_threshold(delay_threshold)
         ,recursion_depth_limit(recursion_depth_limit)
         {
            FC_ASSERT( static_cast<bool>(checktime), "checktime cannot be empty" );
         }

         enum permission_cache_status {
            being_evaluated,
            permission_provided,
            permission_unsatisfied,
            permission_satisfied
         };

         typedef map<permission_level, permission_cache_status> permission_cache_type;

         bool satisfied( const permission_level& permission,
                         fc::microseconds override_delay_threshold,
                         permission_cache_type* cached_perms = nullptr
                       )
         {
            auto delay_threshold_reverter = fc::make_scoped_exit([this, delay = delay_threshold] () mutable {
               delay_threshold = delay;
            });

            delay_threshold = override_delay_threshold;

            return satisfied( permission, cached_perms );
         }

         bool satisfied( const permission_level& permission, permission_cache_type* cached_perms = nullptr ) {
            permission_cache_type cached_permissions;

            if( cached_perms == nullptr )
               cached_perms = initialize_permission_cache( cached_permissions );



            weight_tally_visitor visitor(*this, *cached_perms, 0);
            return ( visitor(permission_level_weight{permission, 1}) > 0 );
         }

         template<typename AuthorityType>
         bool satisfied( const AuthorityType& authority,
                         fc::microseconds override_delay_threshold,
                         permission_cache_type* cached_perms = nullptr
                       )
         {
            auto delay_threshold_reverter = fc::make_scoped_exit([this, delay = delay_threshold] () mutable {
               delay_threshold = delay;
            });

            delay_threshold = override_delay_threshold;

            return satisfied( authority, cached_perms );
         }

         template<typename AuthorityType>
         bool satisfied( const AuthorityType& authority, permission_cache_type* cached_perms = nullptr ) {
            permission_cache_type cached_permissions;

            if( cached_perms == nullptr )
               cached_perms = initialize_permission_cache( cached_permissions );

            return satisfied( authority, *cached_perms, 0 );
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

         PermissionVisitor& get_permission_visitor() {
            return permission_visitor;
         }

         static optional<permission_cache_status>
         permission_status_in_cache( const permission_cache_type& permissions,
                                     const permission_level& level )
         {
            auto itr = permissions.find( level );
            if( itr != permissions.end() )
               return itr->second;

            itr = permissions.find( {level.actor, permission_name()} );
            if( itr != permissions.end() )
               return itr->second;

            return optional<permission_cache_status>();
         }

      private:
         permission_cache_type* initialize_permission_cache( permission_cache_type& cached_permissions ) {
            for( const auto& p : provided_permissions ) {
               cached_permissions.emplace_hint( cached_permissions.end(), p, permission_provided );
            }
            return &cached_permissions;
         }

         template<typename AuthorityType>
         bool satisfied( const AuthorityType& authority, permission_cache_type& cached_permissions, uint16_t depth ) {
            // Save the current used keys; if we do not satisfy this authority, the newly used keys aren't actually used
            auto KeyReverter = fc::make_scoped_exit([this, keys = _used_keys] () mutable {
               _used_keys = keys;
            });

            // Sort key permissions and account permissions together into a single set of meta_permissions
            detail::meta_permission_set permissions;

            permissions.insert(authority.keys.begin(), authority.keys.end());
            permissions.insert(authority.accounts.begin(), authority.accounts.end());

            // Check all permissions, from highest weight to lowest, seeing if signing_keys satisfies them or not
            weight_tally_visitor visitor(*this, cached_permissions, depth);
            for( const auto& permission : permissions )
               // If we've got enough weight, to satisfy the authority, return!
               if( permission.visit(visitor) >= authority.threshold ) {
                  KeyReverter.cancel();
                  return true;
               }
            return false;
         }

         struct weight_tally_visitor {
            using result_type = uint32_t;

            authority_checker&     checker;
            permission_cache_type& cached_permissions;
            uint16_t               recursion_depth;
            uint32_t               total_weight = 0;

            weight_tally_visitor(authority_checker& checker, permission_cache_type& cached_permissions, uint16_t recursion_depth)
            :checker(checker)
            ,cached_permissions(cached_permissions)
            ,recursion_depth(recursion_depth)
            {}

            uint32_t operator()(const key_weight& permission) {
               auto itr = boost::find( checker.signing_keys, permission.key );
               if (itr != checker.signing_keys.end()) {
                  checker._used_keys[itr - checker.signing_keys.begin()] = true;
                  total_weight += permission.weight;
               }
               return total_weight;
            }
            uint32_t operator()(const permission_level_weight& permission) {
               checker.permission_visitor.push_undo();
               checker.permission_visitor( permission.permission );

               auto status = authority_checker::permission_status_in_cache( cached_permissions, permission.permission );
               if( !status ) {
                  if( recursion_depth < checker.recursion_depth_limit ) {
                     bool r = false;
                     typename permission_cache_type::iterator itr = cached_permissions.end();

                     bool propagate_error = false;
                     try {
                        auto&& auth = checker.permission_to_authority( permission.permission );
                        propagate_error = true;
                        if( fc::microseconds(auth.delay_sec) > checker.delay_threshold ) {
                           checker.permission_visitor.pop_undo();
                           return total_weight; // if delay of permission is higher than the threshold, continue without it
                        }
                        auto res = cached_permissions.emplace( permission.permission, being_evaluated );
                        itr = res.first;
                        r = checker.satisfied( std::forward<decltype(auth)>(auth), cached_permissions, recursion_depth + 1 );
                     } catch( const permission_query_exception& ) {
                        checker.permission_visitor.pop_undo();
                        if( propagate_error )
                           throw;
                        else
                           return total_weight; // if the permission doesn't exist, continue without it
                     } catch( ... ) {
                        checker.permission_visitor.pop_undo();
                        throw;
                     }

                     if( r ) {
                        total_weight += permission.weight;
                        itr->second = permission_satisfied;
                        checker.permission_visitor( permission.permission, false );
                        checker.permission_visitor.squash_undo();
                     } else {
                        itr->second = permission_unsatisfied;
                        checker.permission_visitor.pop_undo();
                     }
                  }
               } else if( *status == permission_satisfied ) {
                  total_weight += permission.weight;
                  checker.permission_visitor( permission.permission, true );
                  checker.permission_visitor.squash_undo();
               } else if( *status == permission_provided ) {
                  auto res = cached_permissions.emplace( permission.permission, permission_provided );
                  
                  try {
                     checker.permission_to_authority( permission.permission ); // make sure permission exists
                  } catch( const permission_query_exception& ) {
                     res.first->second = permission_unsatisfied; // do not bother checking again
                     checker.permission_visitor.pop_undo();
                     return total_weight; // if the permission doesn't exist, continue without it
                  }

                  total_weight += permission.weight;
                  res.first->second = permission_satisfied;
                  checker.permission_visitor( permission.permission, false );
                  checker.permission_visitor.squash_undo();
               }
               return total_weight;
            }
         };

   }; /// authority_checker

   template<typename PermissionToAuthorityFunc, typename PermissionVisitor>
   auto make_auth_checker( PermissionToAuthorityFunc&&          pta,
                           PermissionVisitor&&                  permission_visitor,
                           uint16_t                             recursion_depth_limit,
                           const flat_set<public_key_type>&     signing_keys,
                           const flat_set<permission_level>&    provided_permissions = flat_set<permission_level>(),
                           fc::microseconds                     delay_threshold = fc::microseconds::maximum(),
                           const std::function<void(uint32_t)>& _checktime = std::function<void(uint32_t)>()
                         )
   {
      auto noop_checktime = []( uint32_t ) {};
      const auto& checktime = ( static_cast<bool>(_checktime) ? _checktime : noop_checktime );
      return authority_checker< PermissionToAuthorityFunc,
                                PermissionVisitor          >( std::forward<PermissionToAuthorityFunc>(pta),
                                                              std::forward<PermissionVisitor>(permission_visitor),
                                                              recursion_depth_limit,
                                                              signing_keys,
                                                              provided_permissions,
                                                              delay_threshold,
                                                              checktime );
   }

   class noop_permission_visitor {
   public:
      void push_undo()   {}
      void pop_undo()    {}
      void squash_undo() {}
      void operator()(const permission_level& perm_level) {} // Called when entering a permission level (may not exist)
      void operator()(const permission_level& perm_level, bool repeat) {} // Called when permission level was satisfied
      // if repeat == false, the perm_level could possibly not exist; but if repeat == true, then it must exist.

      // Note: permission "existing" assumes that permission_to_authority successfully returns an authority if the permission exists,
      //       and throws a permission_query_exception exception if the permission does not exist.
   };

} } // namespace eosio::chain
