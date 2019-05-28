/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>
#include <boost/asio/io_context.hpp>
#include <future>

namespace boost { namespace asio {
   class thread_pool;
}}

namespace eosio { namespace chain {

class transaction_metadata;
using transaction_metadata_ptr = std::shared_ptr<transaction_metadata>;
using signing_keys_future_value_type = std::tuple<chain_id_type, fc::microseconds, std::shared_ptr<flat_set<public_key_type>>>;
using signing_keys_future_type = std::shared_future<signing_keys_future_value_type>;
using recovery_keys_type = std::pair<fc::microseconds, std::shared_ptr<flat_set<public_key_type>>>;

/**
 *  This data structure should store context-free cached data about a transaction such as
 *  packed/unpacked/compressed and recovered keys
 */
class transaction_metadata {
   private:
      const packed_transaction_ptr                               _packed_trx;
      const transaction_id_type                                  _id;
      const transaction_id_type                                  _signed_id;
      mutable std::mutex                                         _signing_keys_future_mtx;
      mutable signing_keys_future_type                           _signing_keys_future;

   public:
      bool                                                       accepted = false;  // not thread safe
      bool                                                       implicit = false;  // not thread safe
      bool                                                       scheduled = false; // not thread safe

      transaction_metadata() = delete;
      transaction_metadata(const transaction_metadata&) = delete;
      transaction_metadata(transaction_metadata&&) = delete;
      transaction_metadata operator=(transaction_metadata&) = delete;
      transaction_metadata operator=(transaction_metadata&&) = delete;

      explicit transaction_metadata( const signed_transaction& t, uint32_t max_variable_sig_size = UINT32_MAX, packed_transaction::compression_type c = packed_transaction::compression_type::none )
            : _packed_trx( std::make_shared<packed_transaction>( t, c ) )
            , _id( t.id() )
            , _signed_id( digest_type::hash( *_packed_trx ) ) {
         check_variable_sig_size(max_variable_sig_size);
      }

      explicit transaction_metadata( const packed_transaction_ptr& ptrx, uint32_t max_variable_sig_size = UINT32_MAX )
            : _packed_trx( ptrx )
            , _id( ptrx->id() )
            , _signed_id( digest_type::hash( *_packed_trx ) ) {
         check_variable_sig_size(max_variable_sig_size);
      }

      void check_variable_sig_size(uint32_t max) {
         for(const signature_type& sig : _packed_trx->get_signed_transaction().signatures)
            EOS_ASSERT(sig.variable_size() <= max, sig_variable_size_limit_exception, "signature variable length component size (${s}) greater than subjective maximum (${m})", ("s", sig.variable_size())("m", max));
      }

      const packed_transaction_ptr& packed_trx()const { return _packed_trx; }
      const transaction_id_type& id()const { return _id; }
      const transaction_id_type& signed_id()const { return _signed_id; }

      // can be called from any thread. It is recommended that next() immediately post to application thread for
      // future processing since next() will be called from the thread_pool.
      static void start_recover_keys( const transaction_metadata_ptr& mtrx, boost::asio::io_context& thread_pool,
                          const chain_id_type& chain_id, fc::microseconds time_limit,
                          std::function<void()> next = std::function<void()>() );

      // start_recover_keys can be called first to begin key recovery
      // if time_limit of start_recover_keys exceeded (or any other exception) then this can throw
      recovery_keys_type recover_keys( const chain_id_type& chain_id ) const;
};

} } // eosio::chain
