/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/parallel_markers.hpp>

#include <fc/scoped_exit.hpp>

#include <boost/range/algorithm/find.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>

#include <functional>

namespace eosio { namespace chain {

namespace detail {

   // Order of the template types in the static_variant matters to meta_permission_comparator.
   using meta_permission = static_variant<permission_level_weight, key_weight, wait_weight>;

   struct get_weight_visitor {
      using result_type = uint32_t;

      template<typename Permission>
      uint32_t operator()( const Permission& permission ) { return permission.weight; }
   };

   // Orders permissions descending by weight, and breaks ties with Wait permissions being less than
   // Key permissions which are in turn less than Account permissions
   struct meta_permission_comparator {
      bool operator()( const meta_permission& lhs, const meta_permission& rhs ) const {
         get_weight_visitor scale;
         auto lhs_weight = lhs.visit(scale);
         auto lhs_type   = lhs.which();
         auto rhs_weight = rhs.visit(scale);
         auto rhs_type   = rhs.which();
         return std::tie( lhs_weight, lhs_type ) > std::tie( rhs_weight, rhs_type );
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
         PermissionToAuthorityFunc            permission_to_authority;
         const std::function<void()>&         checktime;
         vector<public_key_type>              provided_keys; // Making this a flat_set<public_key_type> causes runtime problems with utilities::filter_data_by_marker for some reason. TODO: Figure out why.
         flat_set<permission_level>           provided_permissions;
         vector<bool>                         _used_keys;
         fc::microseconds                     provided_delay;
         uint16_t                             recursion_depth_limit;

      public:
         authority_checker( PermissionToAuthorityFunc            permission_to_authority,
                            uint16_t                             recursion_depth_limit,
                            const flat_set<public_key_type>&     provided_keys,
                            const flat_set<permission_level>&    provided_permissions,
                            fc::microseconds                     provided_delay,
                            const std::function<void()>&         checktime
                         )
         :permission_to_authority(permission_to_authority)
         ,checktime( checktime )
         ,provided_keys(provided_keys.begin(), provided_keys.end())
         ,provided_permissions(provided_permissions)
         ,_used_keys(provided_keys.size(), false)
         ,provided_delay(provided_delay)
         ,recursion_depth_limit(recursion_depth_limit)
         {
            EOS_ASSERT( static_cast<bool>(checktime), authorization_exception, "checktime cannot be empty" );
         }

         enum permission_cache_status {
            being_evaluated,
            permission_unsatisfied,
            permission_satisfied
         };

         typedef map<permission_level, permission_cache_status> permission_cache_type;

         bool satisfied( const permission_level& permission,
                         fc::microseconds override_provided_delay,
                         permission_cache_type* cached_perms = nullptr
                       )
         {
            auto delay_reverter = fc::make_scoped_exit( [this, delay = provided_delay] () mutable {
               provided_delay = delay;
            });

            provided_delay = override_provided_delay;

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
                         fc::microseconds override_provided_delay,
                         permission_cache_type* cached_perms = nullptr
                       )
         {
            auto delay_reverter = fc::make_scoped_exit( [this, delay = provided_delay] () mutable {
               provided_delay = delay;
            });

            provided_delay = override_provided_delay;

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
            auto range = filter_data_by_marker(provided_keys, _used_keys, true);
            return {range.begin(), range.end()};
         }
         flat_set<public_key_type> unused_keys() const {
            auto range = filter_data_by_marker(provided_keys, _used_keys, false);
            return {range.begin(), range.end()};
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
               cached_permissions.emplace_hint( cached_permissions.end(), p, permission_satisfied );
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

            permissions.insert(authority.waits.begin(), authority.waits.end());
            permissions.insert(authority.keys.begin(), authority.keys.end());
            permissions.insert(authority.accounts.begin(), authority.accounts.end());

            // Check all permissions, from highest weight to lowest, seeing if provided authorization factors satisfies them or not
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

            uint32_t operator()(const wait_weight& permission) {
               if( checker.provided_delay >= fc::seconds(permission.wait_sec) ) {
                  total_weight += permission.weight;
               }
               return total_weight;
            }

            uint32_t operator()(const key_weight& permission) {
               auto itr = boost::find( checker.provided_keys, permission.key );
               if( itr != checker.provided_keys.end() ) {
                  checker._used_keys[itr - checker.provided_keys.begin()] = true;
                  total_weight += permission.weight;
               }
               return total_weight;
            }

            uint32_t operator()(const permission_level_weight& permission) {
               auto status = authority_checker::permission_status_in_cache( cached_permissions, permission.permission );
               if( !status ) {
                  if( recursion_depth < checker.recursion_depth_limit ) {
                     bool r = false;
                     typename permission_cache_type::iterator itr = cached_permissions.end();

                     bool propagate_error = false;
                     try {
                        auto&& auth = checker.permission_to_authority( permission.permission );
                        propagate_error = true;
                        auto res = cached_permissions.emplace( permission.permission, being_evaluated );
                        itr = res.first;
                        r = checker.satisfied( std::forward<decltype(auth)>(auth), cached_permissions, recursion_depth + 1 );
                     } catch( const permission_query_exception& ) {
                        if( propagate_error )
                           throw;
                        else
                           return total_weight; // if the permission doesn't exist, continue without it
                     }

                     if( r ) {
                        total_weight += permission.weight;
                        itr->second = permission_satisfied;
                     } else {
                        itr->second = permission_unsatisfied;
                     }
                  }
               } else if( *status == permission_satisfied ) {
                  total_weight += permission.weight;
               }
               return total_weight;
            }
         };

   }; /// authority_checker

   template<typename PermissionToAuthorityFunc>
   auto make_auth_checker( PermissionToAuthorityFunc&&          pta,
                           uint16_t                             recursion_depth_limit,
                           const flat_set<public_key_type>&     provided_keys,
                           const flat_set<permission_level>&    provided_permissions = flat_set<permission_level>(),
                           fc::microseconds                     provided_delay = fc::microseconds(0),
                           const std::function<void()>&         _checktime = std::function<void()>()
                         )
   {
      auto noop_checktime = []() {};
      const auto& checktime = ( static_cast<bool>(_checktime) ? _checktime : noop_checktime );
      return authority_checker< PermissionToAuthorityFunc>( std::forward<PermissionToAuthorityFunc>(pta),
                                                            recursion_depth_limit,
                                                            provided_keys,
                                                            provided_permissions,
                                                            provided_delay,
                                                            checktime );
   }

} } // namespace eosio::chain
