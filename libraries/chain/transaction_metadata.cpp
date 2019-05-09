#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <boost/asio/thread_pool.hpp>

namespace eosio { namespace chain {

recovery_keys_type transaction_metadata::recover_keys( const chain_id_type& chain_id ) const {
   // Unlikely for more than one chain_id to be used in one nodeos instance
   std::unique_lock<std::mutex> g( _signing_keys_future_mtx );
   if( _signing_keys_future.valid() ) {
      const signing_keys_future_value_type& sig_keys = _signing_keys_future.get();
      if( std::get<0>( sig_keys ) == chain_id ) {
         return std::make_pair( std::get<1>( sig_keys ), std::get<2>( sig_keys ) );
      }
   }

   g.unlock();
   // shared_keys_future not created or different chain_id
   auto recovered_pub_keys = std::make_shared<flat_set<public_key_type>>();
   const signed_transaction& trn = _packed_trx->get_signed_transaction();
   fc::microseconds cpu_usage = trn.get_signature_keys( chain_id, fc::time_point::maximum(), *recovered_pub_keys );

   g.lock();
   std::promise<signing_keys_future_value_type> p;
   p.set_value( std::make_tuple( chain_id, cpu_usage, std::move( recovered_pub_keys ) ) );
   _signing_keys_future = p.get_future().share();

   const signing_keys_future_value_type& sig_keys = _signing_keys_future.get();
   return std::make_pair( std::get<1>( sig_keys ), std::get<2>( sig_keys ) );
}

void transaction_metadata::start_recover_keys( const transaction_metadata_ptr& mtrx,
                                               boost::asio::io_context& thread_pool,
                                               const chain_id_type& chain_id,
                                               fc::microseconds time_limit,
                                               std::function<void()> next )
{
   std::unique_lock<std::mutex> g( mtrx->_signing_keys_future_mtx );
   if( mtrx->_signing_keys_future.valid() && std::get<0>( mtrx->_signing_keys_future.get() ) == chain_id ) { // already created
      g.unlock();
      if( next ) next();
   }

   mtrx->_signing_keys_future = async_thread_pool( thread_pool, [time_limit, chain_id, mtrx, next{std::move(next)}]() {
      fc::time_point deadline = time_limit == fc::microseconds::maximum() ?
                                fc::time_point::maximum() : fc::time_point::now() + time_limit;

      auto recovered_pub_keys = std::make_shared<flat_set<public_key_type>>();
      const signed_transaction& trn = mtrx->_packed_trx->get_signed_transaction();
      fc::microseconds cpu_usage = trn.get_signature_keys( chain_id, deadline, *recovered_pub_keys );
      if( next ) next();
      return std::make_tuple( chain_id, cpu_usage, std::move( recovered_pub_keys ));
   } );

}


} } // eosio::chain
