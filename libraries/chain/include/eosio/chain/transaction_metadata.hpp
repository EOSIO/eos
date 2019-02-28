/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>
#include <future>

namespace boost { namespace asio {
   class thread_pool;
}}

namespace eosio { namespace chain {

class transaction_metadata;
using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;
/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata {
   public:
      transaction_id_type                                        id;
      transaction_id_type                                        signed_id;
      packed_transaction_ptr                                     packed_trx;
      fc::microseconds                                           sig_cpu_usage;
      optional<pair<chain_id_type, flat_set<public_key_type>>>   signing_keys;
      std::future<std::tuple<chain_id_type, fc::microseconds, flat_set<public_key_type>>>
                                                                 signing_keys_future;
      bool                                                       accepted = false;
      bool                                                       implicit = false;
      bool                                                       scheduled = false;

      transaction_metadata() = delete;
      transaction_metadata(const transaction_metadata&) = delete;
      transaction_metadata(transaction_metadata&&) = delete;
      transaction_metadata operator=(transaction_metadata&) = delete;
      transaction_metadata operator=(transaction_metadata&&) = delete;

      explicit transaction_metadata( const signed_transaction& t, packed_transaction::compression_type c = packed_transaction::none )
      :id(t.id()), packed_trx(std::make_shared<packed_transaction>(t, c)) {
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         signed_id = digest_type::hash(*packed_trx);
      }

      explicit transaction_metadata( const packed_transaction_ptr& ptrx )
      :id(ptrx->id()), packed_trx(ptrx) {
         //raw_packed = fc::raw::pack( static_cast<const transaction&>(trx) );
         signed_id = digest_type::hash(*packed_trx);
      }

      const flat_set<public_key_type>& recover_keys( const chain_id_type& chain_id );

      static void create_signing_keys_future( const transaction_metadata_ptr& mtrx, boost::asio::thread_pool& thread_pool,
                                              const chain_id_type& chain_id, fc::microseconds time_limit );

};

} } // eosio::chain
