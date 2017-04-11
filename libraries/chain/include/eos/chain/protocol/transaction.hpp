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
#include <eos/chain/message.hpp>

#include <numeric>

namespace eos { namespace chain {

   /**
    * @defgroup transactions Transactions
    *
    * All transactions are sets of messages that must be applied atomically (all succeed or all fail). Transactions
    * must refer to a recent block that defines the context of the operation so that they assert a known
    * state-precondition assumed by the transaction signers.
    *
    * Rather than specify a full block number, we only specify the lower 16 bits of the block number which means you
    * can reference any block within the last 65,536 blocks which is 2.2 days with a 3 second block interval.
    *
    * All transactions must expire so that the network does not have to maintain a permanent record of all transactions
    * ever published. A transaction may not have an expiration date too far in the future because this would require
    * keeping too much transaction history in memory.
    *
    * The block prefix is the first 4 bytes of the block hash of the reference block number, which is the second 4
    * bytes of the @ref block_id_type (the first 4 bytes of the block ID are the block number)
    *
    * @note It is not recommended to set the @ref ref_block_num, @ref ref_block_prefix, and @ref expiration
    * fields manually. Call @ref set_expiration instead.
    *
    * @{
    */

   /**
    * @brief A transaction is a group of messages that should be applied atomically
    *
    * All interactions the wider world has with the blockchain take place through transactions. To interact with the
    * blockchain, a transaction is created containing messages. The messages specify the changes to be made to the
    * blockchain, and the authorization required to execute the transaction is determined by the messages it contains.
    * All messages in a transaction are executed in order and atomically; i.e. if any message in a transaction is
    * processed, all are, in order, without any other actions taking place between adjacent messages.
    *
    * In practice, unrelated transactions and messages may be evaluated in parallel, but it is guaranteed that this
    * will not be done with transactions which interact with eachother or the same accounts.
    */
   struct transaction
   {
      /**
       * Least significant 16 bits from the reference block's number
       */
      uint16_t ref_block_num = 0;
      /**
       * The first non-block-number 32-bits of the reference block's ID
       *
       * Recall that block IDs have 32 bits of block
       * number followed by the actual block hash, so this field should be set using the second 32 bits in the
       * @ref block_id_type
       */
      uint32_t ref_block_prefix = 0;
      /**
       * This field specifies the absolute expiration time for this transaction
       */
      fc::time_point_sec expiration;

      /**
       * The messages in this transaction
       */
      vector<message> messages;

      /// Calculate the digest for a transaction
      digest_type         digest()const;
      transaction_id_type id()const;
      void                validate() const;
      /// Calculate the digest used for signature validation
      digest_type         sig_digest(const chain_id_type& chain_id)const;

      void set_expiration(fc::time_point_sec expiration_time);
      void set_reference_block(const block_id_type& reference_block);
   };

   /**
    * @brief A single authorization used to authorize a transaction
    *
    * Transactions may have several accounts that must authorize them before they may be evaluated. Each of those
    * accounts has several different authority levels, some of which may be sufficient to authorize the transaction in
    * question and some of which may not. This struct records an account, an the authority level on that account, that
    * must/did authorize a transaction.
    *
    * Objects of this struct shall be added to a @ref signed_transaction when it is being signed, to declare that a
    * given account/authority level has approved the transaction. These permissions will then be checked by blockchain
    * nodes, when receiving the transaction over the network, to verify that the transaction bears signatures
    * corresponding to all of the permissions it declares. Finally, when processing the transaction in context with
    * blockchain state, it will be verified that the transaction declared all of the appropriate permissions.
    */
   struct authorization {
      /// The account authorizing the transaction
      account_name authorizing_account;
      /// The privileges being invoked to authorize the transaction
      privilege_class privileges;
   };

   /**
    * @brief A transaction with signatures
    *
    * signed_transaction is a transaction with an additional manifest of authorizations included with the transaction,
    * and the signatures backing those authorizations.
    */
   struct signed_transaction : public transaction
   {
      signed_transaction(const transaction& trx = transaction())
         : transaction(trx){}

      /** signs and appends to signatures */
      const signature_type& sign(const private_key_type& key, const chain_id_type& chain_id);

      /** returns signature but does not append */
      signature_type sign(const private_key_type& key, const chain_id_type& chain_id)const;

      flat_set<public_key_type> get_signature_keys(const chain_id_type& chain_id)const;

      /**
       * @brief This is the list of signatures that are provided for this transaction
       *
       * These should roughly parallel @ref provided_authorizations below. It is possible that two authorizations may
       * use the same key, however, in which case there would be a single signature for multiple authorizations.
       */
      vector<signature_type> signatures;
      /**
       * @brief This is the list of authorizations that are provided for this transaction
       *
       * Note that this is NOT the set of authorizations that are <i>required</i> for the transaction! The difference
       * is subtle since for a properly authorized transaction, the provided authorizations are the required
       * authorizations, but in practice it must be verified that the provided_authorizations are sufficient to fully
       * authorize the transaction.
       */
      vector<authorization> provided_authorizations;

      /**
       * Removes all messages, signatures, and authorizations
       */
      void clear() { messages.clear(); signatures.clear(); provided_authorizations.clear(); }

      digest_type merkle_digest()const;
   };

   /// @} transactions group

} } // eos::chain

FC_REFLECT(eos::chain::transaction, (ref_block_num)(ref_block_prefix)(expiration)(messages))
FC_REFLECT(eos::chain::authorization, (authorizing_account)(privileges))
FC_REFLECT_DERIVED(eos::chain::signed_transaction, (eos::chain::transaction), (signatures))
