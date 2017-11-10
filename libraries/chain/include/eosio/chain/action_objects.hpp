/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/types.hpp>
#include <eosio/chain/permission_object.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   /**
    *  Maps the permission level on the code to the permission level specififed by owner, when specifying a contract the
    *  contract will specify 1 permission_object per action, and by default the parent of that permission object will be
    *  the active permission of the contract; however, the contract owner could group their actions any way they like.
    *
    *  When it comes time to evaluate whether User can call Action on Contract with UserPermissionLevel the algorithm 
    *  operates as follows:
    *
    *  let scope_permission = action_code.permission
    *  while( ! mapping for (scope_permission / owner )
    *    scope_permission = scope_permission.parent
    *    if( !scope_permission ) 
    *       user permission => active 
    *       break;
    *
    *   Now that we know the required user permission...
    *
    *   while( ! transaction.has( user_permission ) )
    *       user_permission = user_permission.parent
    *       if( !user_permission )
    *          throw invalid permission
    *   pass
    */
   class action_permission_object : public chainbase::object<action_permission_object_type, action_permission_object>
   {
      OBJECT_CTOR(action_permission_object)

      id_type                        id;
      account_name                   owner; ///< the account whose permission we seek
      permission_object::id_type     scope_permission; ///< the scope permission defined by the contract for the action
      permission_object::id_type     owner_permission; ///< the owner permission that is required
   };

   struct by_owner_scope;
   using action_permission_index = chainbase::shared_multi_index_container<
      action_permission_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<action_permission_object, action_permission_object::id_type, &action_permission_object::id>>,
         ordered_unique<tag<by_owner_scope>, 
            composite_key< action_permission_object,
               member<action_permission_object, account_name, &action_permission_object::owner>,
               member<action_permission_object, permission_object::id_type, &action_permission_object::scope_permission>
            >
         >
      >
   >;

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::action_permission_object, eosio::chain::action_permission_index)

FC_REFLECT(eosio::chain::action_permission_object, (id)(owner)(owner_permission)(scope_permission) )
