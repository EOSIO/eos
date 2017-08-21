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

#include "multi_index_includes.hpp"

namespace eos { namespace chain {

   struct by_scope_primary;
   struct by_scope_secondary;
   struct by_scope_tertiary;

   struct key_value_object : public chainbase::object<key_value_object_type, key_value_object> {
      OBJECT_CTOR(key_value_object, (value))
      
      typedef uint64_t key_type;
      static const int number_of_keys = 1;

      id_type               id;
      AccountName           scope; 
      AccountName           code;
      AccountName           table;
      AccountName           primary_key;
      shared_string         value;
   };

   using key_value_index = chainbase::shared_multi_index_container<
      key_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key_value_object, key_value_object::id_type, &key_value_object::id>>,
         ordered_unique<tag<by_scope_primary>, 
            composite_key< key_value_object,
               member<key_value_object, AccountName, &key_value_object::scope>,
               member<key_value_object, AccountName, &key_value_object::code>,
               member<key_value_object, AccountName, &key_value_object::table>,
               member<key_value_object, AccountName, &key_value_object::primary_key>
            >,
            composite_key_compare< std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,std::less<AccountName> >
         >
      >
   >;

   struct key128x128_value_object : public chainbase::object<key128x128_value_object_type, key128x128_value_object> {
      OBJECT_CTOR(key128x128_value_object, (value))

      typedef uint128_t key_type;
      static const int number_of_keys = 2;

      id_type               id;
      AccountName           scope; 
      AccountName           code;
      AccountName           table;
      uint128_t             primary_key;
      uint128_t             secondary_key;
      shared_string         value;
   };

   using key128x128_value_index = chainbase::shared_multi_index_container<
      key128x128_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key128x128_value_object, key128x128_value_object::id_type, &key128x128_value_object::id>>,
         ordered_unique<tag<by_scope_primary>, 
            composite_key< key128x128_value_object,
               member<key128x128_value_object, AccountName, &key128x128_value_object::scope>,
               member<key128x128_value_object, AccountName, &key128x128_value_object::code>,
               member<key128x128_value_object, AccountName, &key128x128_value_object::table>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::primary_key>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::secondary_key>
            >,
            composite_key_compare< std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,std::less<uint128_t>,std::less<uint128_t> >
         >,
         ordered_unique<tag<by_scope_secondary>, 
            composite_key< key128x128_value_object,
               member<key128x128_value_object, AccountName, &key128x128_value_object::scope>,
               member<key128x128_value_object, AccountName, &key128x128_value_object::code>,
               member<key128x128_value_object, AccountName, &key128x128_value_object::table>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::secondary_key>,
               member<key128x128_value_object, uint128_t, &key128x128_value_object::primary_key>
            >,
            composite_key_compare< std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,std::less<uint128_t>,std::less<uint128_t> >
         >
      >
   >;

   struct key64x64x64_value_object : public chainbase::object<key64x64x64_value_object_type, key64x64x64_value_object> {
      OBJECT_CTOR(key64x64x64_value_object, (value))

      typedef uint64_t key_type;
      static const int number_of_keys = 3;

      id_type               id;
      AccountName           scope;
      AccountName           code;
      AccountName           table;
      uint64_t              primary_key;
      uint64_t              secondary_key;
      uint64_t              tertiary_key;
      shared_string         value;
   };

   using key64x64x64_value_index = chainbase::shared_multi_index_container<
      key64x64x64_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key64x64x64_value_object, key64x64x64_value_object::id_type, &key64x64x64_value_object::id>>,
         ordered_unique<tag<by_scope_primary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::scope>,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::code>,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::table>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::primary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::secondary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>
            >,
            composite_key_compare< std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,std::less<uint64_t>,std::less<uint64_t>,std::less<uint64_t> >
         >,
         ordered_unique<tag<by_scope_secondary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::scope>,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::code>,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::table>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::secondary_key>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>
            >,
            composite_key_compare< std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,std::less<uint64_t>,std::less<uint64_t> >
         >,
         ordered_unique<tag<by_scope_tertiary>,
            composite_key< key64x64x64_value_object,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::scope>,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::code>,
               member<key64x64x64_value_object, AccountName, &key64x64x64_value_object::table>,
               member<key64x64x64_value_object, uint64_t, &key64x64x64_value_object::tertiary_key>
            >,
            composite_key_compare< std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,std::less<uint64_t> >
         >         
      >
   >;




} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::key_value_object, eos::chain::key_value_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::key128x128_value_object, eos::chain::key128x128_value_index)
CHAINBASE_SET_INDEX_TYPE(eos::chain::key64x64x64_value_object, eos::chain::key64x64x64_value_index)

FC_REFLECT(eos::chain::key_value_object, (id)(scope)(code)(table)(primary_key)(value) )
