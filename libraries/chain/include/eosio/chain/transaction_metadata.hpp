/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {

/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata {
   public:
      transaction_id_type                   id;
      transaction_id_type                   signed_id;
      signed_transaction                    trx;
      packed_transaction                    packed_trx;
      bytes                                 raw_packed; /// fc::raw::pack(trx)
      bytes                                 raw_packed_with_cfd; /// fc::raw::pack(trx)
      optional<flat_set<public_key_type>>   signing_keys;

      optional<std::function<void(const transaction_trace_ptr&)>>       on_result;


      transaction_metadata( const signed_transaction& t )
      :trx(t),packed_trx(t,packed_transaction::zlib) {
         id = trx.id();
         raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         signed_id = digest_type::hash(raw_packed_with_cfd);
      }

      transaction_metadata( const packed_transaction& ptrx )
      :trx( ptrx.get_signed_transaction() ), packed_trx(ptrx) {
         raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         id = trx.id();
         signed_id = digest_type::hash(raw_packed_with_cfd);
      }

      const flat_set<public_key_type>& recover_keys( chain_id_type cid = chain_id_type() ) {
        return *(signing_keys = trx.get_signature_keys( cid )); 
      }

      uint32_t total_actions()const { return trx.context_free_actions.size() + trx.actions.size(); }
};

typedef std::shared_ptr<transaction_metadata> transaction_metadata_ptr;

} } // eosio::chain

