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

   struct key_value_object : public chainbase::object<key_value_object_type, key_value_object> {
      OBJECT_CTOR(key_value_object, (value))

      id_type               id;
      AccountName           scope; 
      AccountName           code;
      AccountName           table;
      AccountName           key;
      shared_string         value;
   };

   struct by_scope_key;
   using key_value_index = chainbase::shared_multi_index_container<
      key_value_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<key_value_object, key_value_object::id_type, &key_value_object::id>>,
         ordered_unique<tag<by_scope_key>, 
            composite_key< key_value_object,
               member<key_value_object, AccountName, &key_value_object::scope>,
               member<key_value_object, AccountName, &key_value_object::code>,
               member<key_value_object, AccountName, &key_value_object::table>,
               member<key_value_object, AccountName, &key_value_object::key>,
               member<key_value_object, shared_string, &key_value_object::value>
            >,
            composite_key_compare< std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,std::less<AccountName>,chainbase::strcmp_less >
         >
      >
   >;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::key_value_object, eos::chain::key_value_index)

FC_REFLECT(eos::chain::key_value_object, (id)(scope)(code)(table)(key)(value) )
