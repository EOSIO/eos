/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {

/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata {
   public:
      transaction_id_type                   id;
      signed_transaction                    trx;
      packed_transaction                    packed_trx;
      bytes                                 raw_packed; /// fc::raw::pack(trx)
      optional<flat_set<public_key_type>>   signing_keys;

      transaction_metadata( const signed_transaction& t )
      :trx(t),packed_trx(t,packed_transaction::zlib) {
         id = trx.id();
         raw_packed = fc::raw::pack( trx );
      }

      transaction_metadata( const packed_transaction& ptrx )
      :trx( ptrx.get_signed_transaction() ), packed_trx(ptrx) {
         raw_packed = fc::raw::pack( trx );
      }

      void recover_keys( chain_id_type cid = chain_id_type() ) {

      }

      uint32_t total_actions()const { return trx.context_free_actions.size() + trx.actions.size(); }


      /*
      transaction_metadata( const transaction& t, 
                            const time_point& published, 
                            const account_name& sender, 
                            uint128_t sender_id, 
                            const char* raw_data, size_t raw_size, 
                            const optional<time_point>& processing_deadline )
         :id(t.id())
         ,published(published)
         ,sender(sender),sender_id(sender_id),raw_data(raw_data),raw_size(raw_size)
         ,processing_deadline(processing_deadline)
         ,_trx(&t)
      {}

      transaction_metadata( const packed_transaction& t, 
                            chain_id_type chainid, 
                            const time_point& published, 
                            const optional<time_point>& processing_deadline = optional<time_point>(), 
                            bool implicit = false );

      transaction_metadata( transaction_metadata && ) = default;
      transaction_metadata& operator= (transaction_metadata &&) = default;

      // things for packed_transaction
      vector<char>                          packed_trx;
      optional<bytes>                       raw_trx; 

      // packed form to pass to contracts if needed... where do these get set?
      const char*                           raw_data = nullptr;
      size_t                                raw_size = 0;

      optional<transaction>                 decompressed_trx;
      vector<bytes>                         context_free_data;
      digest_type                           packed_digest;

      // things for signed/packed transactions
      optional<flat_set<public_key_type>>   signing_keys;

      transaction_id_type                   id;

      uint32_t                              billable_packed_size  = 0;
      uint32_t                              signature_count       = 0;
      time_point                            published;
      fc::microseconds                      delay;

      // things for processing deferred transactions
      optional<account_name>                sender;
      uint128_t                             sender_id = 0;

      // is this transaction implicit
      bool                                  is_implicit = false;

      //// TODO: ensure _trx is always set right rather than if/else on every query
      const transaction& trx() const{
         FC_ASSERT( _trx != nullptr );
         return *_trx;
      }

      // limits
      optional<time_point>                  processing_deadline;

   private:
      const transaction* _trx = nullptr;
      */
};

typedef std::shared_ptr<transaction_metadata> transaction_metadata_ptr;

} } // eosio::chain

