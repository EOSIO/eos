#define BOOST_TEST_MODULE ordered_exec_trxs
#include <boost/test/included/unit_test.hpp>

#include <eosio/amqp_trx_plugin/fifo_trx_processing_queue.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>

#include <eosio/testing/tester.hpp>

#include <eosio/chain/genesis_state.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/trace.hpp>

#include <appbase/application.hpp>

namespace {

using namespace eosio;
using namespace eosio::chain;

auto make_unique_trx( const chain_id_type& chain_id, fc::time_point expire = fc::time_point::now() + fc::seconds( 120 ) ) {

   static uint64_t nextid = 0;
   ++nextid;

   account_name creator = config::system_account_name;
   auto priv_key = eosio::testing::base_tester::get_private_key( creator, "active" );

   signed_transaction trx;
   trx.expiration = expire;
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             onerror{ nextid, "test", 4 }); // onerror is just a handy action
   trx.sign(priv_key, chain_id);

   return std::make_shared<packed_transaction>( std::move(trx), true, packed_transaction::compression_type::none);
}


struct mock_producer_plugin {
   using retry_later_function_t =
   std::function<void(const chain::transaction_metadata_ptr& trx, producer_plugin::next_function<chain::transaction_trace_ptr>& next)>;

   bool execute_incoming_transaction(const chain::transaction_metadata_ptr& trx,
                                     producer_plugin::next_function<chain::transaction_trace_ptr> next,
                                     retry_later_function_t retry_later) {
      static int num = 0;
      if( ++num % 3 != 0 ) {
         retry_later(trx, next);
         return false;
      } else {
         trxs_.push_back( trx );
         transaction_trace trace;
         trace.id = trx->id();
         next( std::make_shared<transaction_trace>( trace ) );
         return true;
      }
   }

   fc::microseconds get_max_transaction_time() const {
      return fc::microseconds( config::default_max_transaction_cpu_usage );
   }

   bool is_producing_block() const {
      return true;
   }

   bool verify_equal( const std::deque<packed_transaction_ptr>& trxs) {
      if( trxs.size() != trxs_.size() ) {
         elog( "${lhs} != ${rhs}", ("lhs", trxs.size())("rhs", trxs_.size()) );
         return false;
      }
      for( size_t i = 0; i < trxs.size(); ++i ) {
         if( trxs[i]->id() != trxs_[i]->id() ) return false;
      }
      return true;
   }

   std::deque<chain::transaction_metadata_ptr> trxs_;
};

}

BOOST_AUTO_TEST_SUITE(ordered_trxs)

BOOST_AUTO_TEST_CASE(order_mock_producer_plugin) {
   std::thread app_thread( [](){
      appbase::app().exec();
   } );

   mock_producer_plugin mock_prod_plug;
   named_thread_pool thread_pool( "test", 5 );

   auto chain_id = genesis_state().compute_chain_id();

   auto queue =
         std::make_shared<fifo_trx_processing_queue<mock_producer_plugin>>(chain_id,
                                                                           config::default_max_variable_signature_length,
                                                                           true,
                                                                           thread_pool.get_executor(),
                                                                           &mock_prod_plug,
                                                                           10);
   queue->run();
   queue->on_block_start();

   std::deque<packed_transaction_ptr> trxs;
   std::atomic<size_t> next_calls = 0;
   std::atomic<bool> next_trx_match = true;
   for( size_t i = 0; i < 42; ++i ) {
      auto ptrx = make_unique_trx(chain_id);
      trxs.push_back( ptrx );
      queue->push( ptrx,
                   [ptrx, &next_calls, &next_trx_match]( const std::variant<fc::exception_ptr, chain::transaction_trace_ptr>& result ) {
         if( std::get<chain::transaction_trace_ptr>(result)->id != ptrx->id() ) {
            next_trx_match = false;
         }
         ++next_calls;
      } );
   }

   while( !queue->empty() ) {
      usleep(100);
   }

   queue->stop();

   BOOST_CHECK_EQUAL( 42, next_calls );
   BOOST_CHECK( next_trx_match.load() );

   appbase::app().quit();
   queue.reset();
   app_thread.join();

   BOOST_REQUIRE( mock_prod_plug.verify_equal(trxs) );
}

BOOST_AUTO_TEST_SUITE_END()
