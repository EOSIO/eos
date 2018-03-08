/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {

class transaction_metadata {
//   transaction_metadata( const transaction& t )
//      :trx(t)
//      ,id(trx.id()) {}
   public:
      transaction_metadata( const transaction& t, const time_point& published, const account_name& sender, uint32_t sender_id, const char* raw_data, size_t raw_size )
         :id(t.id())
         ,published(published)
         ,sender(sender),sender_id(sender_id),raw_data(raw_data),raw_size(raw_size),_trx(&t)
      {}

      transaction_metadata( const packed_transaction& t, chain_id_type chainid, const time_point& published );

      transaction_metadata( transaction_metadata && ) = default;
      transaction_metadata& operator= (transaction_metadata &&) = default;

      // things for packed_transaction
      optional<bytes>                       raw_trx;
      optional<transaction>                 decompressed_trx;
      vector<bytes>                         context_free_data;

      // things for signed/packed transactions
      optional<flat_set<public_key_type>>   signing_keys;

      transaction_id_type                   id;

      uint32_t                              region_id       = 0;
      uint32_t                              cycle_index     = 0;
      uint32_t                              shard_index     = 0;
      uint32_t                              bandwidth_usage = 0;
      time_point                            published;

      // things for processing deferred transactions
      optional<account_name>                sender;
      uint32_t                              sender_id = 0;

      // packed form to pass to contracts if needed
      const char*                           raw_data = nullptr;
      size_t                                raw_size = 0;

      vector<char>                          packed_trx;

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

      static digest_type calculate_transaction_merkle_root( const vector<transaction_metadata>& metas );
   private:
      const transaction* _trx;
};

} } // eosio::chain

