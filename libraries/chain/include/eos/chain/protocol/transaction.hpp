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
#include <eos/chain/protocol/types.hpp>

#include <numeric>

namespace eos { namespace chain {

   /**
    * @defgroup transactions Transactions
    *
    * All transactions are sets of messages that must be applied atomically (all succeed or all fail). 
    * Transactions must refer to a recent block that defines the context of the operation so that they 
    * assert a known state-precondition assumed by the transaction signers.
    *
    * Rather than specify a full block number, we only specify the lower 16 bits of the block number 
    * which means you can reference any block within the last 65,536 blocks which is 2.2 days with 
    * a 3 second block interval.
    *
    * All transactions must expire so that the network does not have to maintain a 
    * permanent record of all transactions ever published.  A transaction may not have an 
    * expiration date too far in the future because this would require keeping too much 
    * transaction history in memory.
    *
    * The block prefix is the first 4 bytes of the block hash of the reference block number, which is the second 4
    * bytes of the @ref block_id_type (the first 4 bytes of the block ID are the block number)
    *
    * Note: A transaction which selects a reference block cannot be migrated between forks outside the period of
    * ref_block_num.time to (ref_block_num.time + rel_exp * interval). This fact can be used to protect market orders
    * which should specify a relatively short re-org window of perhaps less than 1 minute. Normal payments should
    * probably have a longer re-org window to ensure their transaction can still go through in the event of a momentary
    * disruption in service.
    *
    * @note It is not recommended to set the @ref ref_block_num, @ref ref_block_prefix, and @ref expiration
    * fields manually. Call the appropriate overload of @ref set_expiration instead.
    *
    * @{
    */

   /**
    *  @brief groups operations that should be applied atomically
    */
   struct transaction
   {
      /**
       * Least significant 16 bits from the reference block number. If @ref relative_expiration is zero, this field
       * must be zero as well.
       */
      uint16_t           ref_block_num    = 0;
      /**
       * The first non-block-number 32-bits of the reference block ID. Recall that block IDs have 32 bits of block
       * number followed by the actual block hash, so this field should be set using the second 32 bits in the
       * @ref block_id_type
       */
      uint32_t           ref_block_prefix = 0;

      /**
       * This field specifies the absolute expiration for this transaction.
       */
      fc::time_point_sec expiration;

      vector<operation>  operations;

      /// Calculate the digest for a transaction
      digest_type         digest()const;
      transaction_id_type id()const;
      void                validate() const;
      /// Calculate the digest used for signature validation
      digest_type         sig_digest( const chain_id_type& chain_id )const;

      void set_expiration( fc::time_point_sec expiration_time );
      void set_reference_block( const block_id_type& reference_block );
   };

   /**
    *  @brief adds a signature to a transaction
    */
   struct signed_transaction : public transaction
   {
      signed_transaction( const transaction& trx = transaction() )
         : transaction(trx){}

      /** signs and appends to signatures */
      const signature_type& sign( const private_key_type& key, const chain_id_type& chain_id );

      /** returns signature but does not append */
      signature_type sign( const private_key_type& key, const chain_id_type& chain_id )const;

      flat_set<public_key_type> get_signature_keys( const chain_id_type& chain_id )const;

      vector<signature_type> signatures;

      /// Removes all operations and signatures
      void clear() { operations.clear(); signatures.clear(); }

      digest_type merkle_digest()const;
   };

   /// @} transactions group

} } // eos::chain

FC_REFLECT( eos::chain::transaction, (ref_block_num)(ref_block_prefix)(expiration)(operations) )
FC_REFLECT_DERIVED( eos::chain::signed_transaction, (eos::chain::transaction), (signatures) )
