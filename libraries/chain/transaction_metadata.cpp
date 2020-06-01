#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <boost/asio/thread_pool.hpp>

namespace eosio { namespace chain {

recover_keys_future transaction_metadata::start_recover_keys( packed_transaction_ptr trx,
                                                              boost::asio::io_context& thread_pool,
                                                              const chain_id_type& chain_id,
                                                              fc::microseconds time_limit,
                                                              uint32_t max_variable_sig_size )
{
   return async_thread_pool( thread_pool, [trx{std::move(trx)}, chain_id, time_limit, max_variable_sig_size]() mutable {
         fc::time_point deadline = time_limit == fc::microseconds::max() ?
                                   fc::time_point::max() : fc::now() + time_limit;
         const vector<signature_type>& sigs = check_variable_sig_size( trx, max_variable_sig_size );
         const vector<bytes>* context_free_data = trx->get_context_free_data();
         EOS_ASSERT( context_free_data, tx_no_context_free_data, "context free data pruned from packed_transaction" );
         flat_set<public_key_type> recovered_pub_keys;
         const bool allow_duplicate_keys = false;
         fc::microseconds cpu_usage =
               trx->get_transaction().get_signature_keys(sigs, chain_id, deadline, *context_free_data, recovered_pub_keys, allow_duplicate_keys);
         return std::make_shared<transaction_metadata>( private_type(), std::move( trx ), cpu_usage, std::move( recovered_pub_keys ) );
      }
   );
}

uint32_t transaction_metadata::get_estimated_size() const {
   return sizeof(*this) + _recovered_pub_keys.size() * sizeof(public_key_type) + packed_trx()->get_estimated_size();
}

} } // eosio::chain
