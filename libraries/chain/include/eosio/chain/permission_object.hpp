/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/authority.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {
   class permission_object : public chainbase::object<permission_object_type, permission_object> {
      OBJECT_CTOR(permission_object, (auth) )

      id_type           id;
      account_name      owner; ///< the account this permission belongs to
      id_type           parent; ///< parent permission
      permission_name   name; ///< human-readable name for the permission
      shared_authority  auth; ///< authority required to execute this permission
      time_point        last_updated; ///< the last time this authority was updated
      fc::microseconds  delay; ///< delay associated with this permission


      /**
       * @brief Checks if this permission is equivalent or greater than other
       * @tparam Index The permission_index
       * @return a fc::microseconds set to the maximum delay encountered between this and the permission that is other;
       * empty optional otherwise
       *
       * Permissions are organized hierarchically such that a parent permission is strictly more powerful than its
       * children/grandchildren. This method checks whether this permission is of greater or equal power (capable of
       * satisfying) permission @ref other. The returned value is an optional<fc::microseconds> that will indicate the
       * maximum delay encountered walking the hierarchy between this permission and other, if this satisfies other,
       * otherwise an empty optional is returned.
       */
      template <typename Index>
      optional<fc::microseconds> satisfies(const permission_object& other, const Index& permission_index) const {
         // If the owners are not the same, this permission cannot satisfy other
         if( owner != other.owner )
            return optional<fc::microseconds>();

         // if this permission satisfies other, then other's delay and this delay will have to contribute
         auto max_delay = other.delay > delay ? other.delay : delay;
         // If this permission matches other, or is the immediate parent of other, then this permission satisfies other
         if( id == other.id || id == other.parent )
            return optional<fc::microseconds>(max_delay);
         // Walk up other's parent tree, seeing if we find this permission. If so, this permission satisfies other
         const permission_object* parent = &*permission_index.template get<by_id>().find(other.parent);
         while( parent ) {
            if( max_delay < parent->delay )
               max_delay = parent->delay;
            if( id == parent->parent )
               return optional<fc::microseconds>(max_delay);
            if( parent->parent._id == 0 )
               return optional<fc::microseconds>();
            parent = &*permission_index.template get<by_id>().find(parent->parent);
         }
         // This permission is not a parent of other, and so does not satisfy other
         return optional<fc::microseconds>();
      }
   };

   struct by_parent;
   struct by_owner;
   struct by_name;
   using permission_index = chainbase::shared_multi_index_container<
      permission_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<permission_object, permission_object::id_type, &permission_object::id>>,
         ordered_unique<tag<by_parent>,
            composite_key<permission_object,
               member<permission_object, permission_object::id_type, &permission_object::parent>,
               member<permission_object, permission_object::id_type, &permission_object::id>
            >
         >,
         ordered_unique<tag<by_owner>,
            composite_key<permission_object,
               member<permission_object, account_name, &permission_object::owner>,
               member<permission_object, permission_name, &permission_object::name>
            >
         >,
         ordered_unique<tag<by_name>,
            composite_key<permission_object,
               member<permission_object, permission_name, &permission_object::name>,
               member<permission_object, permission_object::id_type, &permission_object::id>
            >
         >
      >
   >;

   class permission_usage_object : public chainbase::object<permission_usage_object_type, permission_usage_object> {
      OBJECT_CTOR(permission_usage_object)

      id_type           id;
      account_name      account;     ///< the account this permission belongs to
      permission_name   permission;  ///< human-readable name for the permission
      time_point        last_used;   ///< when this permission was last used
   };

   struct by_account_permission;
   using permission_usage_index = chainbase::shared_multi_index_container<
      permission_usage_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<permission_usage_object, permission_usage_object::id_type, &permission_usage_object::id>>,
         ordered_unique<tag<by_account_permission>,
            composite_key<permission_usage_object,
               member<permission_usage_object, account_name, &permission_usage_object::account>,
               member<permission_usage_object, permission_name, &permission_usage_object::permission>,
               member<permission_usage_object, permission_usage_object::id_type, &permission_usage_object::id>
            >
         >
      >
   >;

   namespace config {
      template<>
      struct billable_size<permission_object> {
         static const uint64_t  overhead = 6 * overhead_per_row_per_index_ram_bytes; ///< 6 indices 2x internal ID, parent, owner, name, name_usage
         static const uint64_t  value = 80 + overhead;  ///< fixed field size + overhead
      };
   }
} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::permission_object, eosio::chain::permission_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::permission_usage_object, eosio::chain::permission_usage_index)

FC_REFLECT(chainbase::oid<eosio::chain::permission_object>, (_id))
FC_REFLECT(eosio::chain::permission_object, (id)(owner)(parent)(name)(auth)(last_updated)(delay))

FC_REFLECT(chainbase::oid<eosio::chain::permission_usage_object>, (_id))
FC_REFLECT(eosio::chain::permission_usage_object, (id)(account)(permission)(last_used))
