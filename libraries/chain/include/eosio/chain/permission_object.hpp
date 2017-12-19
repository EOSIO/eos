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

      /**
       * @brief Checks if this permission is equivalent or greater than other
       * @tparam Index The permission_index
       * @return True if this permission matches, or is a parent of, other; false otherwise
       *
       * Permissions are organized hierarchically such that a parent permission is strictly more powerful than its
       * children/grandchildren. This method checks whether this permission is of greater or equal power (capable of
       * satisfying) permission @ref other. This would be the case if this permission matches, or is some parent of,
       * other.
       */
      template <typename Index>
      bool satisfies(const permission_object& other, const Index& permission_index) const {
         // If the owners are not the same, this permission cannot satisfy other
         if (owner != other.owner)
            return false;
         // If this permission matches other, or is the immediate parent of other, then this permission satisfies other
         if (id == other.id || id == other.parent)
            return true;
         // Walk up other's parent tree, seeing if we find this permission. If so, this permission satisfies other
         const permission_object* parent = &*permission_index.template get<by_id>().find(other.parent);
         while (parent) {
            if (id == parent->parent)
               return true;
            if (parent->parent._id == 0)
               return false;
            parent = &*permission_index.template get<by_id>().find(parent->parent);
         }
         // This permission is not a parent of other, and so does not satisfy other
         return false;
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

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::permission_object, eosio::chain::permission_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::permission_usage_object, eosio::chain::permission_usage_index)

FC_REFLECT(chainbase::oid<eosio::chain::permission_object>, (_id))
FC_REFLECT(eosio::chain::permission_object, (id)(owner)(parent)(name)(auth))

FC_REFLECT(chainbase::oid<eosio::chain::permission_usage_object>, (_id))
FC_REFLECT(eosio::chain::permission_usage_object, (id)(account)(permission)(last_used))
