#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <boost/asio/thread_pool.hpp>

namespace eosio { namespace chain {


const flat_set<public_key_type>& transaction_metadata::recover_keys( const chain_id_type& chain_id ) {
   // Unlikely for more than one chain_id to be used in one nodeos instance
   if( !signing_keys || signing_keys->first != chain_id ) {
      if( signing_keys_future.valid() ) {
         std::tuple<chain_id_type, fc::microseconds, flat_set<public_key_type>> sig_keys = signing_keys_future.get();
         if( std::get<0>( sig_keys ) == chain_id ) {
            sig_cpu_usage = std::get<1>( sig_keys );
            signing_keys.emplace( std::get<0>( sig_keys ), std::move( std::get<2>( sig_keys )));
            return signing_keys->second;
         }
      }
      flat_set<public_key_type> recovered_pub_keys;
      sig_cpu_usage = packed_trx->get_signed_transaction().get_signature_keys( chain_id, fc::time_point::maximum(), recovered_pub_keys );
      signing_keys.emplace( chain_id, std::move( recovered_pub_keys ));
   }
   return signing_keys->second;
}

void transaction_metadata::create_signing_keys_future( const transaction_metadata_ptr& mtrx,
      boost::asio::thread_pool& thread_pool, const chain_id_type& chain_id, fc::microseconds time_limit ) {
   if( mtrx->signing_keys.valid() ) // already created
      return;

   std::weak_ptr<transaction_metadata> mtrx_wp = mtrx;
   mtrx->signing_keys_future = async_thread_pool( thread_pool, [time_limit, chain_id, mtrx_wp]() {
      fc::time_point deadline = time_limit == fc::microseconds::maximum() ?
            fc::time_point::maximum() : fc::time_point::now() + time_limit;
      auto mtrx = mtrx_wp.lock();
      fc::microseconds cpu_usage;
      flat_set<public_key_type> recovered_pub_keys;
      if( mtrx ) {
         const signed_transaction& trn = mtrx->packed_trx->get_signed_transaction();
         cpu_usage = trn.get_signature_keys( chain_id, deadline, recovered_pub_keys );
      }
      return std::make_tuple( chain_id, cpu_usage, std::move( recovered_pub_keys ));
   } );
}


} } // eosio::chain
