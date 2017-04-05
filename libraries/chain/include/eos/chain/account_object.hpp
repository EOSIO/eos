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
         account_object() = delete;
   public:
         template<typename Constructor, typename Allocator>
         account_object(Constructor&& c, chainbase::allocator<Allocator> a)
            : name(a) {
            c(*this);
         }

         id_type           id;
         shared_string     name;
         public_key_type   owner_key;
         public_key_type   active_key;
   };

   struct by_name;
   using account_multi_index = chainbase::shared_multi_index_container<
      account_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<account_object, account_object::id_type, &account_object::id>>,
         ordered_unique<tag<by_name>, member<account_object, shared_string, &account_object::name>,
                        chainbase::strcmp_less>
      >
   >;
} } // eos::chain

CHAINBASE_SET_INDEX_TYPE(eos::chain::account_object, eos::chain::account_multi_index)

FC_REFLECT(eos::chain::account_object,
           (id)
           (name)
           (owner_key)
           (active_key)
          )
