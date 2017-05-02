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
#include <eos/chain/authority.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {

   struct shared_authority {
      shared_authority( chainbase::allocator<char> alloc )
      :accounts(alloc),keys(alloc)
      {}

      shared_authority& operator=(const Authority& a) {
         threshold = a.threshold;
         accounts = decltype(accounts)(a.accounts.begin(), a.accounts.end(), accounts.get_allocator());
         keys = decltype(keys)(a.keys.begin(), a.keys.end(), keys.get_allocator());
         return *this;
      }
      shared_authority& operator=(Authority&& a) {
         threshold = a.threshold;
         accounts.reserve(a.accounts.size());
         for (auto& p : a.accounts)
            accounts.emplace_back(std::move(p));
         keys.reserve(a.keys.size());
         for (auto& p : a.keys)
            keys.emplace_back(std::move(p));
         return *this;
      }

      UInt32                                        threshold = 0;
      shared_vector<types::AccountPermissionWeight> accounts;
      shared_vector<types::KeyPermissionWeight>     keys;
   };
   
   class account_object : public chainbase::object<account_object_type, account_object>
   {
      OBJECT_CTOR(account_object)

      id_type           id;
      AccountName       name;
      Asset             balance;
      UInt64            votes                  = 0;
      UInt64            converting_votes       = 0;
      Time              last_vote_conversion;
   };

   struct by_name;
   using account_index = chainbase::shared_multi_index_container<
      account_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<account_object, account_object::id_type, &account_object::id>>,
         ordered_unique<tag<by_name>, member<account_object, AccountName, &account_object::name>>
      >
   >;

   class permission_object : public chainbase::object<permission_object_type, permission_object>
   {
      OBJECT_CTOR(permission_object, (auth) )

      id_type           id;
      account_id_type   owner; ///< the account this permission belongs to
      id_type           parent; ///< parent permission 
      PermissionName   name;
      shared_authority  auth; ///< TODO
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
               member<permission_object, PermissionName, &permission_object::name>,
               member<permission_object, permission_object::id_type, &permission_object::id>
            >
         >,
         ordered_unique<tag<by_name>, member<permission_object, PermissionName, &permission_object::name> >
      >
   >;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::account_object, eos::chain::account_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::permission_object, eos::chain::permission_index)

FC_REFLECT(eos::chain::account_object, (id)(name)(balance)(votes)(converting_votes)(last_vote_conversion) )
FC_REFLECT(eos::chain::permission_object, (id)(owner)(parent)(name) )
