#pragma once
#include <eosio/blockchain/transaction.hpp>

namespace eosio { namespace blockchain {


   /**
    * <h2>Proof-of-message and Proof-of-complete-transcript</h2>
    * 
    * Each block header will include a merkle tree root which can be used to efficiently 
    * prove that a message or transaction has been processed by the primary chain.
    * 
    * Additionally, it will be possible to recursively prove that a transcript of messages
    * affecting an entry in the database is complete (it has no ommissions )
    * 
    * Given a block number N
    * BlockMerkle(N) ->
    *    MerkleTree( SummaryMerkle(N),  MerkleTree([BlockMerkle(P)] | P <- [0..N) ]) )
    *
    * SummaryMerkle(N) ->
    *    MerkleTree([ CycleMerkle(C,N) | C <- [0..BlockData[N].cycles.size()) ])
    *
    * CycleMerkle(C, N) ->
    *    MerkleTree([ ShardMerkle(S,C,N) | S <- [0..BlockData[N].cycles[C].shards.size()) ])
    *
    * ShardMerkle(S, C, N) ->
    *    MerkleTree([ TxMerkle(T,S,C,N) | T <- [0..BlockData[N].cycles[C].shards[S].transactions.size())  ])
    *
    * TxMerkle(T,S,C,N) ->
    *    MerkleTree([ BlockData[N].cycles[C].shards[S].transactions[T].tx_id, TxProofsMerkle(T,S,C,N) ])
    * 
    * TxProofsMerkle(T,S,C,N) ->
    *    let tx = BlockData[N].cycles[C].shards[S].transactions[T];
    *    MerkleTree([ MessageProof(M,T,S,C,N) | M <- [0..tx.messageOutputs.size() ], 
    *              ,[ GeneratedProof(G,T,S,C,N) | G <- [0..tx.generatedTransactions.size() ])
    *
    * MessageProof(M,T,S,C,N) ->
    *    let { code, func, data, reciever, scope_code_versions } <- BlockData[N].cycles[C].shards[S].transactions[T].messageOutputs[M];
    *    Hash(code, func, data, reciever, scope_code_versions, N, C, S, R, M)
    *    
    * GeneratedProof(G,T,S,C,N) ->
    *    let { tx, parent_index } <- BlockData[N].cycles[C].shards[S].transactions[T].generatedTransactions[G];
    *    Hash(tx, parent_index, G)
    *
    * <h2>What is a "Scope_code_version"?</h2>
    * 
    * For each tuple of (scope, code) eos will track a monotonically increasing version number.
    * This version number increments AFTER processing any message that gave write access (regardless of mutation) to the given code
    *
    * The version prior to execution is recorded as part of the message output.  This allows construction of a provable complete 
    * transcript of messages that may have affected a given scopes data.  For instance, if you wanted to prove that there was
    * no material transactions between the reciept of funds and a further transfer of said funds you can produce a set of message proofs
    * that completely documents the messages that affect the funded account scope from the receipts version number to the transfers version.
    *
    * Any gap in versions indicates an ommission of data.  
    *
    *
    */

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
       * Used to prove side effects of processing messages
       */
      merkle_id_type                message_root;

      /**
       * The merkle root of all block ids, this is kept outside the merkl
       */
      merkle_id_type                block_root;

      /**
       * Account name of the producer that signed this block for the purpose of
       * looking up the expected public key.
       */
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
      typedef vector<transaction_receipt>   shard; /// new or generated transactions 
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
      typedef vector<meta_transaction_ptr>     shard;
      typedef vector<shard>                    cycle;
                                         
      vector<cycle>                            cycles;
   };
   typedef shared_ptr<block_data> block_data_ptr;

   /**
    *  Tracks high-level information about pending blocks and how they link.
    */
   struct meta_block : std::enable_shared_from_this<meta_block> {
      meta_block( block_data_ptr p ):id(p->id()),previous_id(p->previous),data(move(p)){}

      weak_ptr< meta_block > prev; ///< weak ptr to the previous block
      bool                   invalid  = false; ///< used to flag invalid blocks and prevent others from building on top
      block_num_type         blocknum = 0;
      block_id_type          id;
      block_id_type          previous_id;
      block_data_ptr         data; ///< the deserialized version of the block, this may be null
      public_key_type        producer_key; ///< the recovered producer key
      vector<char>           packed; ///<< includes the serialized version of the packed_block
   };

   typedef shared_ptr<meta_block> meta_block_ptr;


   /**
    * A packed block makes it effecient to read blocks from disk without having to dynamically allocate all of the
    * individual cycles, shards, transactions, messages, and message data. Instead it simply contains the block header
    * along with a buffer containing the packed_cycles.
    */
   struct packed_block : public signed_block_header {
      vector<char> packed_cycles; ///< contains block_summary::cycles and block_data::cycles 

      meta_block_ptr to_meta_block()const;
   };

} } /// eosio::blockchain

FC_REFLECT( eosio::blockchain::block_header, (previous)(timestamp)(summary_root)(message_root)(block_root)(producer)(producer_changes) )
FC_REFLECT_DERIVED( eosio::blockchain::signed_block_header, (eosio::blockchain::block_header), (producer_signature) )
FC_REFLECT_DERIVED( eosio::blockchain::block_summary, (eosio::blockchain::signed_block_header), (cycles) )
FC_REFLECT_DERIVED( eosio::blockchain::block_data, (eosio::blockchain::block_summary), (cycles) )
FC_REFLECT_DERIVED( eosio::blockchain::packed_block, (eosio::blockchain::signed_block_header), (packed_cycles) )



