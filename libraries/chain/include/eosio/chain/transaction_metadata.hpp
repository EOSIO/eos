/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {

struct transaction_metadata {
   transaction_metadata( const transaction& t )
      :trx(t)
      ,id(trx.id()) {}

   transaction_metadata( const transaction& t, const time_point& published, const account_name& sender, uint32_t sender_id, const char* generated_data, size_t generated_size )
      :trx(t)
      ,id(trx.id())
      ,published(published)
      ,sender(sender),sender_id(sender_id),generated_data(generated_data),generated_size(generated_size)
   {}

   transaction_metadata( const signed_transaction& t, chain_id_type chainid, const time_point& published )
      :trx(t)
      ,id(trx.id())
      ,bandwidth_usage( fc::raw::pack_size(t) )
      ,published(published)
   { }

   const transaction&                    trx;
   transaction_id_type                   id;
   optional<flat_set<public_key_type>>   signing_keys;
   uint32_t                              region_id       = 0;
   uint32_t                              cycle_index     = 0;
   uint32_t                              shard_index     = 0;
   uint32_t                              bandwidth_usage = 0;
   time_point                            published;

   // things for processing deferred transactions
   optional<account_name>                sender;
   uint32_t                              sender_id = 0;
   const char*                           generated_data = nullptr;
   size_t                                generated_size = 0;

   // scopes available to this transaction if we are applying a block
   optional<const vector<shard_lock>*>   allowed_read_locks;
   optional<const vector<shard_lock>*>   allowed_write_locks;
};

} } // eosio::chain

