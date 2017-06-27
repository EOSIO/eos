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

   struct type_object : public chainbase::object<type_object_type, type_object> {
      OBJECT_CTOR(type_object, (fields))

      id_type               id;
      AccountName           scope;
      TypeName              name;
      AccountName           base_scope;
      TypeName              base;
      shared_vector<Field>  fields;
   };
   using type_id_type = type_object::id_type;

   struct by_scope_name;
   using type_index = chainbase::shared_multi_index_container<
      type_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<type_object, type_object::id_type, &type_object::id>>,
         ordered_unique<tag<by_scope_name>, 
            composite_key< type_object,
               member<type_object, AccountName, &type_object::scope>,
               member<type_object, TypeName, &type_object::name>
            >
         >
      >
   >;

} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::type_object, eos::chain::type_index)

FC_REFLECT(chainbase::oid<eos::chain::type_object>, (_id))

FC_REFLECT(eos::chain::type_object, (id)(scope)(name)(base_scope)(base)(fields) )
