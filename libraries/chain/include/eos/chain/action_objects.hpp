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
#include <eos/chain/types.hpp>
#include <eos/chain/account_object.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {

   /**
    * This table defines all of the event handlers for every contract.  Every message is
    * delivered TO a particular contract and also processed in parallel by several other contracts. 
    *
    * Each account can define a custom handler based upon the tuple { processor, recipient, type } where
    * processor is the account that is processing the message, recipient is the account specified by
    * message::recipient and type is messagse::type.
    * 
    *
    */
   class action_code_object : public chainbase::object<action_code_object_type, action_code_object>
   {
      OBJECT_CTOR(action_code_object, (validate_action)(validate_precondition)(apply) )

      id_type                        id;
      account_id_type                recipient;
      account_id_type                processor; 
      TypeName                       type; ///< the name of the action (defines serialization)

      shared_string   validate_action; ///< read only access to action
      shared_string   validate_precondition; ///< read only access to state
      shared_string   apply; ///< the code that executes the state transition
   };

   struct by_parent;
   struct by_processor_recipient_type;
   using action_code_index = chainbase::shared_multi_index_container<
      action_code_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<action_code_object, action_code_object::id_type, &action_code_object::id>>,
         ordered_unique<tag<by_processor_recipient_type>, 
            composite_key< action_code_object,
               member<action_code_object, account_id_type, &action_code_object::processor>,
               member<action_code_object, account_id_type, &action_code_object::recipient>,
               member<action_code_object, TypeName, &action_code_object::type>
            >
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

CHAINBASE_SET_INDEX_TYPE(eos::chain::action_code_object, eos::chain::action_code_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::action_permission_object, eos::chain::action_permission_index)

FC_REFLECT(eos::chain::action_code_object, (id)(recipient)(processor)(type)(validate_action)(validate_precondition)(apply) )
FC_REFLECT(eos::chain::action_permission_object, (id)(owner)(owner_permission)(scope_permission) )
