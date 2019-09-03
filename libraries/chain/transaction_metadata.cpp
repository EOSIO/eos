#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <boost/asio/thread_pool.hpp>

namespace eosio { namespace chain {

recover_keys_future transaction_metadata::start_recover_keys( const packed_transaction_ptr& trx,
                                                              boost::asio::io_context& thread_pool,
                                                              const chain_id_type& chain_id,
                                                              fc::microseconds time_limit,
                                                              uint32_t max_variable_sig_size )
{
   return async_thread_pool( thread_pool, [trx, chain_id, time_limit, max_variable_sig_size]() {
         fc::time_point deadline = time_limit == fc::microseconds::maximum() ?
                                   fc::time_point::maximum() : fc::time_point::now() + time_limit;
         check_variable_sig_size( trx, max_variable_sig_size );
         const signed_transaction& trn = trx->get_signed_transaction();
         flat_set<public_key_type> recovered_pub_keys;
         fc::microseconds cpu_usage = trn.get_signature_keys( chain_id, deadline, recovered_pub_keys );
         return std::make_shared<transaction_metadata>( private_type(), trx, cpu_usage, std::move( recovered_pub_keys ) );
      }
   );
}

} } // eosio::chain
