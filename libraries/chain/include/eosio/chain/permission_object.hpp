/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/authority.hpp>
#include <eosio/chain/multi_index_includes.hpp>
#include <eosio/chain/database_utils.hpp>

namespace eosio { namespace chain {

   class permission_usage_object : public cyberway::chaindb::object<permission_usage_object_type, permission_usage_object> {
      CHAINDB_OBJECT_ID_CTOR(permission_usage_object)

      id_type           id;
      time_point        last_used;   ///< when this permission was last used
   };

   struct by_account_permission;
   using permission_usage_table = cyberway::chaindb::table_container<
      permission_usage_object,
      cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(permission_usage_object, permission_usage_object::id_type, id)>
      >
   >;


   class permission_object : public cyberway::chaindb::object<permission_object_type, permission_object> {
      CHAINDB_OBJECT_ID_CTOR(permission_object)

      id_type                           id;
      permission_usage_object::id_type  usage_id;
      id_type                           parent; ///< parent permission
      account_name                      owner; ///< the account this permission belongs to
      permission_name                   name; ///< human-readable name for the permission
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
         auto itr = permission_index.find(other.parent._id);
         while( permission_index.end() != itr ) {
            if( id == itr->parent )
               return true;
            if( itr->parent._id == 0 )
               return false;
            itr = permission_index.find(itr->parent._id);
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
   using permission_table = cyberway::chaindb::table_container<
      permission_object,
       cyberway::chaindb::indexed_by<
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(permission_object, permission_object::id_type, id)>,
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_parent>,
            cyberway::chaindb::composite_key<permission_object,
               BOOST_MULTI_INDEX_MEMBER(permission_object, permission_object::id_type, parent),
               BOOST_MULTI_INDEX_MEMBER(permission_object, permission_object::id_type, id)
            >
         >,
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_owner>,
           cyberway::chaindb::composite_key<permission_object,
               BOOST_MULTI_INDEX_MEMBER(permission_object, account_name, owner),
               BOOST_MULTI_INDEX_MEMBER(permission_object, permission_name, name)
            >
         >,
         cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_name>,
            cyberway::chaindb::composite_key<permission_object,
               BOOST_MULTI_INDEX_MEMBER(permission_object, permission_name, name),
               BOOST_MULTI_INDEX_MEMBER(permission_object, permission_object::id_type, id)
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

CHAINDB_SET_TABLE_TYPE(eosio::chain::permission_object, eosio::chain::permission_table)
CHAINDB_TAG(eosio::chain::permission_object, permission)
CHAINDB_SET_TABLE_TYPE(eosio::chain::permission_usage_object, eosio::chain::permission_usage_table)
CHAINDB_TAG(eosio::chain::permission_usage_object, permusage)

FC_REFLECT(eosio::chain::permission_object, (id)(usage_id)(parent)(owner)(name)(last_updated)(auth))

FC_REFLECT(eosio::chain::permission_usage_object, (id)(last_used))
FC_REFLECT(eosio::chain::snapshot_permission_object, (parent)(owner)(name)(last_updated)(last_used)(auth))

