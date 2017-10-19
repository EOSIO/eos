#pragma once
#include <eosio/blockchain/transaction.hpp>

namespace eosio { namespace blockchain {

   struct block_header
   {
      block_id_type   digest() const;
      uint32_t        block_num() const { return num_from_id(previous) + 1; }
      static uint32_t num_from_id(const block_id_type& id);


      block_id_type                 previous;
      fc::time_point                timestamp;

      /**
       * Used to prove inputs to block as intended by producer 
       */
      merkle_id_type                summary_root;
      /**
       * Used to prove side effects of processing blocks
       */
      merkle_id_type                merkle_root;

      account_name                  producer;

      /**
       * This defines a change in the set of active producers which 
       * will take effect after this block is deemed irreversible by
       * virtue of 2/3 confirmation by existing producers. 
       *
       * We include the public key with producer changes so that anyone can
       * validate the blockchain based only on the block headers and some simple
       * state based upon a mapping of producer names to keys. 
       *
       * Must be stored with keys *and* values sorted, thus this is a valid RoundChanges:
       * [["A", ["X",KEY]],
       *  ["B", ["Y",KEY]]]
       * ... whereas this is not:
       * [["A", ["Y",KEY]],
       *  ["B", ["X",KEY]]]
       * Even though the above examples are semantically equivalent (replace A and B with X and Y), only the first is
       * legal.
       */
      producer_changes_type      producer_changes;
   };

   /**
    *  The purpose of this class is to add the producer's signature and to separate it for
    *  hash calculation purposes.
    */
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
      typedef vector<transaction_id_type>   shard; /// new or generated transactions 
      typedef vector<shard>                 cycle;

      vector<cycle>               cycles;
   };
   typedef shared_ptr<block_summary> block_summary_ptr;


   /**
    * This structure contains the set of signed transactions referenced by the
    * block summary. This inherits from block_summary/signed_block_header and is
    * what would be logged to disk to enable the regeneration of blockchain state.
    *
    * The transactions are grouped to mirror the cycles in block_summary, generated
    * transactions are not included.  
    */
   struct block_data : public block_summary {
      typedef vector<signed_transaction_ptr> shard;
      typedef vector<shard>                  cycle;
                                         
      vector<cycle>                          cycles;
   };
   typedef shared_ptr<block_data> block_data_ptr;


} } /// eosio::blockchain

FC_REFLECT( eosio::blockchain::block_header, (previous)(timestamp)(summary_root)(merkle_root)(producer)(producer_changes) )
FC_REFLECT_DERIVED( eosio::blockchain::signed_block_header, (eosio::blockchain::block_header), (producer_signature) )
FC_REFLECT_DERIVED( eosio::blockchain::block_summary, (eosio::blockchain::signed_block_header), (cycles) )
FC_REFLECT_DERIVED( eosio::blockchain::block_data, (eosio::blockchain::block_summary), (cycles) )



