/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/action.hpp>
#include <numeric>

namespace eosio { namespace chain {

   /**
    *  The transaction header contains the fixed-sized data
    *  associated with each transaction. It is separated from
    *  the transaction body to facilitate partial parsing of
    *  transactions without requiring dynamic memory allocation.
    *
    *  All transactions have an expiration time after which they
    *  may no longer be included in the blockchain. Once a block
    *  with a block_header::timestamp greater than expiration is
    *  deemed irreversible, then a user can safely trust the transaction
    *  will never be included.
    *

    *  Each region is an independent blockchain, it is included as routing
    *  information for inter-blockchain communication. A contract in this
    *  region might generate or authorize a transaction intended for a foreign
    *  region.
    */
   struct transaction_header {
      time_point_sec         expiration;   ///< the time at which a transaction expires
      uint16_t               ref_block_num       = 0U; ///< specifies a block num in the last 2^16 blocks.
      uint32_t               ref_block_prefix    = 0UL; ///< specifies the lower 32 bits of the blockid at get_ref_blocknum
      fc::unsigned_int       max_net_usage_words = 0UL; /// upper limit on total network bandwidth (in 8 byte words) billed for this transaction
      uint8_t                max_cpu_usage_ms    = 0; /// upper limit on the total CPU time billed for this transaction
      fc::unsigned_int       delay_sec           = 0UL; /// number of seconds to delay this transaction for during which it may be canceled.

      /**
       * @return the absolute block number given the relative ref_block_num
       */
      block_num_type get_ref_blocknum( block_num_type head_blocknum )const {
         return ((head_blocknum/0xffff)*0xffff) + head_blocknum%0xffff;
      }
      void set_reference_block( const block_id_type& reference_block );
      bool verify_reference_block( const block_id_type& reference_block )const;
      void validate()const;
   };

   /**
    *  A transaction consits of a set of messages which must all be applied or
    *  all are rejected. These messages have access to data within the given
    *  read and write scopes.
    */
   struct transaction : public transaction_header {
      vector<action>         context_free_actions;
      vector<action>         actions;
      extensions_type        transaction_extensions;

      transaction_id_type        id()const;
      digest_type                sig_digest( const chain_id_type& chain_id, const vector<bytes>& cfd = vector<bytes>() )const;
      flat_set<public_key_type>  get_signature_keys( const vector<signature_type>& signatures,
                                                     const chain_id_type& chain_id,
                                                     const vector<bytes>& cfd = vector<bytes>(),
                                                     bool allow_duplicate_keys = false,
                                                     bool use_cache = true )const;

      uint32_t total_actions()const { return context_free_actions.size() + actions.size(); }
      account_name first_authorizor()const {
         for( const auto& a : actions ) {
            for( const auto& u : a.authorization )
               return u.actor;
         }
         return account_name();
      }

   };

   struct signed_transaction : public transaction
   {
      signed_transaction() = default;
//      signed_transaction( const signed_transaction& ) = default;
//      signed_transaction( signed_transaction&& ) = default;
      signed_transaction( transaction&& trx, const vector<signature_type>& signatures, const vector<bytes>& context_free_data)
      : transaction(std::move(trx))
      , signatures(signatures)
      , context_free_data(context_free_data)
      {
      }

      vector<signature_type>    signatures;
      vector<bytes>             context_free_data; ///< for each context-free action, there is an entry here

      const signature_type&     sign(const private_key_type& key, const chain_id_type& chain_id);
      signature_type            sign(const private_key_type& key, const chain_id_type& chain_id)const;
      flat_set<public_key_type> get_signature_keys( const chain_id_type& chain_id, bool allow_duplicate_keys = false, bool use_cache = true )const;
   };

   struct packed_transaction {
      enum compression_type {
         none = 0,
         zlib = 1,
      };

      packed_transaction() = default;

      explicit packed_transaction(const transaction& t, compression_type _compression = none)
      {
         set_transaction(t, _compression);
      }

      explicit packed_transaction(const signed_transaction& t, compression_type _compression = none)
      :signatures(t.signatures)
      {
         set_transaction(t, t.context_free_data, _compression);
      }

      explicit packed_transaction(signed_transaction&& t, compression_type _compression = none)
      :signatures(std::move(t.signatures))
      {
         set_transaction(t, std::move(t.context_free_data), _compression);
      }

      uint32_t get_unprunable_size()const;
      uint32_t get_prunable_size()const;

      digest_type packed_digest()const;

      vector<signature_type>                  signatures;
      fc::enum_type<uint8_t,compression_type> compression;
      bytes                                   packed_context_free_data;
      bytes                                   packed_trx;

      time_point_sec     expiration()const;
      transaction_id_type id()const;
      bytes              get_raw_transaction()const;
      vector<bytes>      get_context_free_data()const;
      transaction        get_transaction()const;
      signed_transaction get_signed_transaction()const;
      void               set_transaction(const transaction& t, compression_type _compression = none);
      void               set_transaction(const transaction& t, const vector<bytes>& cfd, compression_type _compression = none);

   private:
      mutable optional<transaction>           unpacked_trx; // <-- intermediate buffer used to retrieve values
      void local_unpack()const;
   };

   using packed_transaction_ptr = std::shared_ptr<packed_transaction>;

   /**
    *  When a transaction is generated it can be scheduled to occur
    *  in the future. It may also fail to execute for some reason in
    *  which case the sender needs to be notified. When the sender
    *  sends a transaction they will assign it an ID which will be
    *  passed back to the sender if the transaction fails for some
    *  reason.
    */
   struct deferred_transaction : public signed_transaction
   {
      uint128_t      sender_id; /// ID assigned by sender of generated, accessible via WASM api when executing normal or error
      account_name   sender; /// receives error handler callback
      account_name   payer;
      time_point_sec execute_after; /// delayed execution

      deferred_transaction() = default;

      deferred_transaction(uint128_t sender_id, account_name sender, account_name payer,time_point_sec execute_after,
                           const signed_transaction& txn)
      : signed_transaction(txn),
        sender_id(sender_id),
        sender(sender),
        payer(payer),
        execute_after(execute_after)
      {}
   };

   struct deferred_reference {
      deferred_reference(){}
      deferred_reference( const account_name& sender, const uint128_t& sender_id)
      :sender(sender),sender_id(sender_id)
      {}

      account_name   sender;
      uint128_t      sender_id;
   };

   uint128_t transaction_id_to_sender_id( const transaction_id_type& tid );

} } /// namespace eosio::chain

FC_REFLECT( eosio::chain::transaction_header, (expiration)(ref_block_num)(ref_block_prefix)
                                              (max_net_usage_words)(max_cpu_usage_ms)(delay_sec) )
FC_REFLECT_DERIVED( eosio::chain::transaction, (eosio::chain::transaction_header), (context_free_actions)(actions)(transaction_extensions) )
FC_REFLECT_DERIVED( eosio::chain::signed_transaction, (eosio::chain::transaction), (signatures)(context_free_data) )
FC_REFLECT_ENUM( eosio::chain::packed_transaction::compression_type, (none)(zlib))
FC_REFLECT( eosio::chain::packed_transaction, (signatures)(compression)(packed_context_free_data)(packed_trx) )
FC_REFLECT_DERIVED( eosio::chain::deferred_transaction, (eosio::chain::signed_transaction), (sender_id)(sender)(payer)(execute_after) )
FC_REFLECT( eosio::chain::deferred_reference, (sender)(sender_id) )
