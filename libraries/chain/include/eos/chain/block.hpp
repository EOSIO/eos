/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/block_timestamp.hpp>
#include <eos/chain/transaction.hpp>

namespace eosio { namespace chain {

   struct block_header
   {
      digest_type     digest() const;
      uint32_t        block_num() const { return num_from_id(previous) + 1; }
      static uint32_t num_from_id(const block_id_type& id);

      block_id_type                 previous;
      block_timestamp_type          timestamp;

      checksum_type                 transaction_mroot; /// mroot of cycles_summary
      checksum_type                 message_mroot;
      checksum_type                 block_mroot;

      account_name                  producer;
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
      optional<producer_schedule_type>  new_producers;
   };

   struct signed_block_header : public block_header
   {
      block_id_type              id() const;
      fc::ecc::public_key        signee() const;
      void                       sign(const fc::ecc::private_key& signer);
      bool                       validate_signee(const fc::ecc::public_key& expected_signee) const;

      signature_type             producer_signature;
   };


   /**
    *  The block_summary defines the set of transactions that were successfully applied as they
    *  are organized into cycles and shards. A shard contains the set of transactions IDs which
    *  are either user generated transactions or code-generated transactions.
    *
    *
    *  The primary purpose of a block is to define the order in which messages are processed.
    *
    *  The secodnary purpose of a block is certify that the messages are valid according to 
    *  a group of 3rd party validators (producers).
    *
    *  The next purpose of a block is to enable light-weight proofs that a transaction occured
    *  and was considered valid.
    *
    *  The next purpose is to enable code to generate messages that are certified by the
    *  producers to be authorized. 
    *
    *  A block is therefore defined by the ordered set of executed and generated transactions,
    *  and the merkle proof is over set of messages delivered as a result of executing the
    *  transactions. 
    *
    *  A message is defined by { receiver, code, function, permission, data }, the merkle
    *  tree of a block should be generated over a set of message IDs rather than a set of
    *  transaction ids. 
    */
   struct block_summary : public signed_block_header {
      typedef vector<transaction_receipt>   shard; /// new or generated transactions 
      typedef vector<shard>                 cycle;

      vector<cycle>    cycles_summary;
   };

   /**
    * This structure contains the set of signed transactions referenced by the
    * block summary. This inherits from block_summary/signed_block_header and is
    * what would be logged to disk to enable the regeneration of blockchain state.
    *
    * The transactions are grouped to mirror the cycles in block_summary, generated
    * transactions are not included.  
    */
   struct signed_block : public block_summary {
      vector<signed_transaction>   input_transactions; /// this is loaded and indexed into map<id,trx> that is referenced by summary
   };


} } // eosio::chain

FC_REFLECT(eosio::chain::block_header, (previous)(timestamp)(transaction_merkle_root)(producer)(producer_changes))
FC_REFLECT_DERIVED(eosio::chain::signed_block_header, (eosio::chain::block_header), (producer_signature))
FC_REFLECT(eosio::chain::thread, (generated_input)(user_input) )
FC_REFLECT_DERIVED(eosio::chain::signed_block, (eosio::chain::signed_block_header), (cycles))
