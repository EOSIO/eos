#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <boost/asio/thread_pool.hpp>

namespace eosio { namespace chain {


const flat_set<public_key_type>& transaction_metadata::recover_keys( const chain_id_type& chain_id ) {
   // Unlikely for more than one chain_id to be used in one nodeos instance
   if( !signing_keys || signing_keys->first != chain_id ) {
      if( signing_keys_future.valid() ) {
         signing_keys = signing_keys_future.get();
         if( signing_keys->first == chain_id ) {
            return signing_keys->second;
         }
      }
      signing_keys = std::make_pair( chain_id, trx.get_signature_keys( chain_id ));
   }
   return signing_keys->second;
}

void transaction_metadata::create_signing_keys_future( const transaction_metadata_ptr& mtrx,
                                                       boost::asio::thread_pool& thread_pool, const chain_id_type& chain_id ) {
   if( mtrx->signing_keys && mtrx->signing_keys->first == chain_id )
      return;

   if( mtrx->signing_keys.valid() ) // already created
      return;

   std::weak_ptr<transaction_metadata> mtrx_wp = mtrx;
   mtrx->signing_keys_future = async_thread_pool( thread_pool, [chain_id, mtrx_wp]() {
      auto mtrx = mtrx_wp.lock();
      return mtrx ?
             std::make_pair( chain_id, mtrx->trx.get_signature_keys( chain_id ) ) :
             std::make_pair( chain_id, decltype( mtrx->trx.get_signature_keys( chain_id ) ){} );
   } );
}


} } // eosio::chain
