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
#include <fc/io/raw.hpp>

#include <eos/chain/transaction.hpp>
#include <fc/uint128.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "multi_index_includes.hpp"

namespace eos { namespace chain {
   using boost::multi_index_container;
   using namespace boost::multi_index;
   /**
    * The purpose of this object is to enable the detection of duplicate transactions. When a transaction is included
    * in a block a transaction_object is added. At the end of block processing all transaction_objects that have
    * expired can be removed from the index.
    */
   class transaction_object : public chainbase::object<transaction_object_type, transaction_object>
   {
         OBJECT_CTOR(transaction_object)

         id_type             id;
         SignedTransaction  trx;
         transaction_id_type trx_id;

         time_point_sec get_expiration()const { return trx.expiration; }
   };

   struct by_expiration;
   struct by_trx_id;
   using transaction_multi_index = chainbase::shared_multi_index_container<
      transaction_object,
      indexed_by<
         ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(transaction_object, transaction_object::id_type, id)>,
         hashed_unique<tag<by_trx_id>, BOOST_MULTI_INDEX_MEMBER(transaction_object, transaction_id_type, trx_id), std::hash<transaction_id_type>>,
         ordered_non_unique<tag<by_expiration>, const_mem_fun<transaction_object, time_point_sec, &transaction_object::get_expiration>>
      >
   >;

   typedef chainbase::generic_index<transaction_multi_index> transaction_index;
} }

CHAINBASE_SET_INDEX_TYPE(eos::chain::transaction_object, eos::chain::transaction_multi_index)

FC_REFLECT( eos::chain::transaction_object, (trx)(trx_id) )
