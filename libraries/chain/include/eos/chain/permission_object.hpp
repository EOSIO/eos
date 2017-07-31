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
   class permission_object : public chainbase::object<permission_object_type, permission_object> {
      OBJECT_CTOR(permission_object, (auth) )

      id_type           id;
      AccountName       owner; ///< the account this permission belongs to
      id_type           parent; ///< parent permission 
      PermissionName    name; ///< human-readable name for the permission
      shared_authority  auth; ///< authority required to execute this permission
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
               member<permission_object, AccountName, &permission_object::owner>,
               member<permission_object, PermissionName, &permission_object::name>
            >
         >,
         ordered_unique<tag<by_name>,
            composite_key<permission_object,
               member<permission_object, PermissionName, &permission_object::name>,
               member<permission_object, permission_object::id_type, &permission_object::id>
            >
         >
      >
   >;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::permission_object, eos::chain::permission_index)

FC_REFLECT(chainbase::oid<eos::chain::permission_object>, (_id))
FC_REFLECT(eos::chain::permission_object, (id)(owner)(parent)(name)(auth))
