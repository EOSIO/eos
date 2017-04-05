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
#include <eos/chain/protocol/operations.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace eos { namespace chain {

   /**
    * @brief tracks the history of all logical operations on blockchain state
    * @ingroup object
    * @ingroup implementation
    *
    *  All operations and virtual operations result in the creation of an
    *  operation_history_object that is maintained on disk as a stack.  Each
    *  real or virtual operation is assigned a unique ID / sequence number that
    *  it can be referenced by.
    *
    *  @note  by default these objects are not tracked, the account_history_plugin must
    *  be loaded fore these objects to be maintained.
    *
    *  @note  this object is READ ONLY it can never be modified
    */
   class operation_history_object : public chainbase::object<operation_history_object_type, operation_history_object>
   {
      public:
         operation_history_object( const operation& o ):op(o){}
         operation_history_object(){}

         operation         op;
         /** the block that caused this operation */
         uint32_t          block_num = 0;
         /** the transaction in the block */
         uint16_t          trx_in_block = 0;
         /** the operation within the transaction */
         uint16_t          op_in_trx = 0;
         /** any virtual operations implied by operation in block */
         uint16_t          virtual_op = 0;
   };

} } // eos::chain

FC_REFLECT( eos::chain::operation_history_object,
                    (op)(block_num)(trx_in_block)(op_in_trx)(virtual_op) )
