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
#include <eos/chain/authority.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {
   /**
    * @brief The permission_link_object class assigns permission_objects to message types
    *
    * This class records the links from contracts and message types within those contracts to permission_objects
    * defined by the user to record the authority required to execute those messages within those contracts. For
    * example, suppose we have a contract called "currency" and that contract defines a message called "transfer".
    * Furthermore, suppose a user, "joe", has a permission level called "money" and joe would like to require that
    * permission level in order for his account to invoke currency.transfer. To do this, joe would create a
    * permission_link_object for his account with "currency" as the code account, "transfer" as the message_type, and
    * "money" as the required_permission. After this, in order to validate, any message to "currency" of type
    * "transfer" that requires joe's approval would require signatures sufficient to satisfy joe's "money" authority.
    */
   class permission_link_object : public chainbase::object<permission_link_object_type, permission_link_object> {
      OBJECT_CTOR(permission_link_object)

      id_type        id;
      AccountName    account;
      AccountName    code;
      FuncName       message_type;
      PermissionName required_permission;
   };

   struct by_message_type;
   struct by_permission_name;
   using permission_link_index = chainbase::shared_multi_index_container<
      permission_link_object,
      indexed_by<
         ordered_unique<tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(permission_link_object, permission_link_object::id_type, id)
         >,
         ordered_unique<tag<by_message_type>,
            composite_key<permission_link_object,
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, AccountName, account),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, AccountName, code),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, FuncName, message_type)
            >
         >,
         ordered_unique<tag<by_permission_name>,
            composite_key<permission_link_object,
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, AccountName, account),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, PermissionName, required_permission),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, AccountName, code),
               BOOST_MULTI_INDEX_MEMBER(permission_link_object, FuncName, message_type)
            >
         >
      >
   >;
} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::permission_link_object, eos::chain::permission_link_index)

FC_REFLECT(eos::chain::permission_link_object, (id)(account)(code)(message_type)(required_permission))
