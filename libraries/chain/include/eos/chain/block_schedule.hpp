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
#include <eos/chain/chain_controller.hpp>
#include <eos/chain/transaction.hpp>

namespace eos { namespace chain {

   struct thread_schedule {
      vector<generated_transaction_id_type> generated_input;
      vector<SignedTransaction const *> user_input;
   };

   using cycle_schedule = vector<thread_schedule>;

   /**
    *   @class block_schedule
    *   @brief represents a proposed order of execution for a generated block
    */
   struct block_schedule
   {
      vector<cycle_schedule> cycles;

      // Algorithms
      
      /**
       * A greedy scheduler that attempts to make short threads to resolve scope contention before 
       * falling back on cycles
       * @return the block scheduler
       */
      static block_schedule by_threading_conflicts(deque<SignedTransaction> const &transactions, const global_property_object& properties);

      /**
       * A greedy scheduler that attempts uses future cycles to resolve scope contention
       * @return the block scheduler
       */
      static block_schedule by_cycling_conflicts(deque<SignedTransaction> const &transactions, const global_property_object& properties);
   };

} } // eos::chain

FC_REFLECT(eos::chain::thread_schedule, (generated_input)(user_input) )
FC_REFLECT(eos::chain::block_schedule, (cycles))
