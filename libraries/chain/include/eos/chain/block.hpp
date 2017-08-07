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
#include <eos/chain/transaction.hpp>

namespace eos { namespace chain {

   struct block_header
   {
      digest_type                   digest() const;
      uint32_t                      block_num() const { return num_from_id(previous) + 1; }
      static uint32_t num_from_id(const block_id_type& id);


      block_id_type                 previous;
      fc::time_point_sec            timestamp;
      checksum_type                 transaction_merkle_root;
      AccountName                   producer;
      /**
       * The changes in the round of producers after this block
       *
       * Must be stored with keys *and* values sorted, thus this is a valid RoundChanges:
       * [["A", "X"],
       *  ["B", "Y"]]
       * ... whereas this is not:
       * [["A", "Y"],
       *  ["B", "X"]]
       * Even though the above examples are semantically equivalent (replace A and B with X and Y), only the first is
       * legal.
       */
      RoundChanges                  producer_changes;
   };

   struct signed_block_header : public block_header
   {
      block_id_type              id() const;
      fc::ecc::public_key        signee() const;
      void                       sign(const fc::ecc::private_key& signer);
      bool                       validate_signee(const fc::ecc::public_key& expected_signee) const;

      signature_type             producer_signature;
   };

   struct thread {
      vector<ProcessedGeneratedTransaction> generated_input;
      vector<ProcessedTransaction>          user_input;

      digest_type merkle_digest() const;
   };

   using cycle = vector<thread>;

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root() const;
      vector<cycle> cycles;
   };

} } // eos::chain

FC_REFLECT(eos::chain::block_header, (previous)(timestamp)(transaction_merkle_root)(producer)(producer_changes))
FC_REFLECT_DERIVED(eos::chain::signed_block_header, (eos::chain::block_header), (producer_signature))
FC_REFLECT(eos::chain::thread, (generated_input)(user_input) )
FC_REFLECT_DERIVED(eos::chain::signed_block, (eos::chain::signed_block_header), (cycles))
