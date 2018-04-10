/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {

class transaction_metadata {
   public:
      transaction_metadata( const transaction& t, const time_point& published, const account_name& sender, uint128_t sender_id, const char* raw_data, size_t raw_size, const optional<time_point>& processing_deadline )
         :id(t.id())
         ,published(published)
         ,sender(sender),sender_id(sender_id),raw_data(raw_data),raw_size(raw_size)
         ,processing_deadline(processing_deadline)
         ,_trx(&t)
      {}

      transaction_metadata( const packed_transaction& t, chain_id_type chainid, const time_point& published, const optional<time_point>& processing_deadline = optional<time_point>(), bool implicit=false );

      transaction_metadata( transaction_metadata && ) = default;
      transaction_metadata& operator= (transaction_metadata &&) = default;

      // things for packed_transaction
      optional<bytes>                       raw_trx;
      optional<transaction>                 decompressed_trx;
      vector<bytes>                         context_free_data;
      digest_type                           packed_digest;

      // things for signed/packed transactions
      optional<flat_set<public_key_type>>   signing_keys;

      transaction_id_type                   id;

      uint32_t                              region_id             = 0;
      uint32_t                              cycle_index           = 0;
      uint32_t                              shard_index           = 0;
      uint32_t                              billable_packed_size  = 0;
      uint32_t                              signature_count       = 0;
      time_point                            published;
      fc::microseconds                      delay;

      // things for processing deferred transactions
      optional<account_name>                sender;
      uint128_t                             sender_id = 0;

      // packed form to pass to contracts if needed
      const char*                           raw_data = nullptr;
      size_t                                raw_size = 0;

      vector<char>                          packed_trx;

      // is this transaction implicit
      bool                                  is_implicit = false;

      // scopes available to this transaction if we are applying a block
      optional<const vector<shard_lock>*>   allowed_read_locks;
      optional<const vector<shard_lock>*>   allowed_write_locks;

      const transaction& trx() const{
         if (decompressed_trx) {
            return *decompressed_trx;
         } else {
            return *_trx;
         }
      }

      // limits
      optional<time_point>                  processing_deadline;

   private:
      const transaction* _trx = nullptr;
};

} } // eosio::chain

FC_REFLECT( eosio::chain::transaction_metadata, (raw_trx)(signing_keys)(id)(region_id)(cycle_index)(shard_index)(billable_packed_size)(published)(sender)(sender_id)(is_implicit))
