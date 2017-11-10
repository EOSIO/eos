/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/chain/transaction.hpp>

namespace eosio { namespace chain {

   struct block_header
   {
      digest_type                   digest() const;
      uint32_t                      block_num() const { return num_from_id(previous) + 1; }
      static uint32_t num_from_id(const block_id_type& id);


      block_id_type                 previous;
      fc::time_point_sec            timestamp;
      checksum_type                 transaction_merkle_root;
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
      round_changes                  producer_changes;
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
      vector<processed_generated_transaction> generated_input;
      vector<processed_transaction>          user_input;

      digest_type merkle_digest() const;
   };

   using cycle = vector<thread>;

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root() const;
      vector<cycle> cycles;
   };

} } // eosio::chain

FC_REFLECT(eosio::chain::block_header, (previous)(timestamp)(transaction_merkle_root)(producer)(producer_changes))
FC_REFLECT_DERIVED(eosio::chain::signed_block_header, (eosio::chain::block_header), (producer_signature))
FC_REFLECT(eosio::chain::thread, (generated_input)(user_input) )
FC_REFLECT_DERIVED(eosio::chain::signed_block, (eosio::chain::signed_block_header), (cycles))
