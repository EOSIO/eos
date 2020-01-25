#pragma once
#include <eosio/chain/authority.hpp>
#include <eosio/chain/database_utils.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   class permission_usage_object : public chainbase::object<permission_usage_object_type, permission_usage_object> {
      OBJECT_CTOR(permission_usage_object)

      id_type           id;
      time_point        last_used;   ///< when this permission was last used
   };

   struct by_account_permission;
   using permission_usage_index = chainbase::shared_multi_index_container<
      permission_usage_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<permission_usage_object, permission_usage_object::id_type, &permission_usage_object::id>>
      >
   >;


   class permission_object : public chainbase::object<permission_object_type, permission_object> {
      OBJECT_CTOR(permission_object, (auth) )

      id_type                           id;
      permission_usage_object::id_type  usage_id;
      id_type                           parent; ///< parent permission
      account_name                      owner; ///< the account this permission belongs to (should not be changed within a chainbase modifier lambda)
      permission_name                   name; ///< human-readable name for the permission (should not be changed within a chainbase modifier lambda)
      time_point                        last_updated; ///< the last time this authority was updated
      shared_authority                  auth; ///< authority required to execute this permission


      /**
       * @brief Checks if this permission is equivalent or greater than other
       * @tparam Index The permission_index
       * @return true if this permission is equivalent or greater than other, false otherwise
       *
       * Permissions are organized hierarchically such that a parent permission is strictly more powerful than its
       * children/grandchildren. This method checks whether this permission is of greater or equal power (capable of
       * satisfying) permission @ref other.
       */
      template <typename Index>
      bool satisfies(const permission_object& other, const Index& permission_index) const {
         // If the owners are not the same, this permission cannot satisfy other
         if( owner != other.owner )
            return false;

         // If this permission matches other, or is the immediate parent of other, then this permission satisfies other
         if( id == other.id || id == other.parent )
            return true;

         // Walk up other's parent tree, seeing if we find this permission. If so, this permission satisfies other
         const permission_object* parent = &*permission_index.template get<by_id>().find(other.parent);
         while( parent ) {
            if( id == parent->parent )
               return true;
            if( parent->parent._id == 0 )
               return false;
            parent = &*permission_index.template get<by_id>().find(parent->parent);
         }
         // This permission is not a parent of other, and so does not satisfy other
         return false;
      }
   };

   /**
    * special cased to abstract the foreign keys for usage and the optimization of using OID for the parent
    */
   struct snapshot_permission_object {
      permission_name   parent; ///< parent permission
      account_name      owner; ///< the account this permission belongs to
      permission_name   name; ///< human-readable name for the permission
      time_point        last_updated; ///< the last time this authority was updated
      time_point        last_used; ///< when this permission was last used
      authority         auth; ///< authority required to execute this permission
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

   namespace config {
      template<>
      struct billable_size<permission_object> { // Also counts memory usage of the associated permission_usage_object
         static const uint64_t  overhead = 5 * overhead_per_row_per_index_ram_bytes; ///< 5 indices 2x internal ID, parent, owner, name
         static const uint64_t  value = (config::billable_size_v<shared_authority> + 64) + overhead;  ///< fixed field size + overhead
      };
   }
} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::permission_object, eosio::chain::permission_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::permission_usage_object, eosio::chain::permission_usage_index)

FC_REFLECT(eosio::chain::permission_object, (usage_id)(parent)(owner)(name)(last_updated)(auth))
FC_REFLECT(eosio::chain::snapshot_permission_object, (parent)(owner)(name)(last_updated)(last_used)(auth))

FC_REFLECT(eosio::chain::permission_usage_object, (last_used))
