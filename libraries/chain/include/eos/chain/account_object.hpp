/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <eos/chain/protocol/types.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {
   class account_object : public chainbase::object<account_object_type, account_object>
   {
      OBJECT_CTOR(account_object, (name))

      id_type           id;
      shared_string     name;
   };

   struct by_name;
   using account_index = chainbase::shared_multi_index_container<
      account_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<account_object, account_object::id_type, &account_object::id>>,
         ordered_unique<tag<by_name>, member<account_object, shared_string, &account_object::name>,
                        chainbase::strcmp_less>
      >
   >;

   class permission_object : public chainbase::object<permission_object_type, permission_object>
   {
      OBJECT_CTOR(permission_object, (name))

      id_type         id;
      account_id_type owner; ///< the account this permission belongs to
      id_type         parent; ///< parent permission 
      shared_string   name;
#warning TODO - add shared_authority to permission object
     // shared_authority auth; ///< TODO
   };



   struct by_parent;
   struct by_owner;
   using permission_index = chainbase::shared_multi_index_container<
      permission_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<permission_object, permission_object::id_type, &permission_object::id>>,
         ordered_unique<tag<by_parent>, 
            composite_key< permission_object,
               member<permission_object, permission_object::id_type, &permission_object::parent>,
               member<permission_object, permission_object::id_type, &permission_object::id>
            >
         >,
         ordered_unique<tag<by_owner>, 
            composite_key< permission_object,
               member<permission_object, account_object::id_type, &permission_object::owner>,
               member<permission_object, permission_object::id_type, &permission_object::id>
            >
         >,
         ordered_unique<tag<by_name>, member<permission_object, shared_string, &permission_object::name>, chainbase::strcmp_less>
      >
   >;


   /**
    * This table defines all of the event handlers for every contract
    */
   class action_code_object : public chainbase::object<action_code_object_type, action_code_object>
   {
      OBJECT_CTOR(action_code_object, (action)(validate_action)(validate_precondition)(apply) )

      id_type                        id;
      account_id_type                scope;
      permission_object::id_type     permission;

#warning TODO: convert action name to fixed with string
      shared_string   action; ///< the name of the action (defines serialization)
      shared_string   validate_action; ///< read only access to action
      shared_string   validate_precondition; ///< read only access to state
      shared_string   apply; ///< the code that executes the state transition
   };

   struct by_parent;
   struct by_scope_action;
   using action_code_index = chainbase::shared_multi_index_container<
      action_code_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<action_code_object, action_code_object::id_type, &action_code_object::id>>,
         ordered_unique<tag<by_scope_action>, 
            composite_key< action_code_object,
               member<action_code_object, account_id_type, &action_code_object::scope>,
               member<action_code_object, shared_string, &action_code_object::action>
            >,
            composite_key_compare< std::less<account_id_type>, chainbase::strcmp_less >
         >
      >
   >;


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
      account_id_type                owner; ///< the account whose permission we seek
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
               member<action_permission_object, account_id_type, &action_permission_object::owner>,
               member<action_permission_object, permission_object::id_type, &action_permission_object::scope_permission>
            >
         >
      >
   >;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::account_object, eos::chain::account_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::permission_object, eos::chain::permission_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::action_code_object, eos::chain::action_code_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::action_permission_object, eos::chain::action_permission_index)

FC_REFLECT(eos::chain::account_object, (id)(name))
FC_REFLECT(eos::chain::permission_object, (id)(owner)(parent)(name) )
FC_REFLECT(eos::chain::action_code_object, (id)(scope)(permission)(action)(validate_action)(validate_precondition)(apply) )
FC_REFLECT(eos::chain::action_permission_object, (id)(owner)(owner_permission)(scope_permission) )
