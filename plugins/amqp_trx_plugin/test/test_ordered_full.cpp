#define BOOST_TEST_MODULE ordered_full_exec_trxs
#include <boost/test/included/unit_test.hpp>

#include <eosio/amqp_trx_plugin/fifo_trx_processing_queue.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>

#include <eosio/testing/tester.hpp>

#include <eosio/chain/genesis_state.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/trace.hpp>

#include <appbase/application.hpp>

namespace eosio::test::detail {
struct testit {
   uint64_t      id;

   testit( uint64_t id = 0 )
         :id(id){}

   static account_name get_account() {
      return chain::config::system_account_name;
   }

   static action_name get_name() {
      return N(testit);
   }
};
}
FC_REFLECT( eosio::test::detail::testit, (id) )

namespace {

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::test::detail;

auto make_unique_trx( const chain_id_type& chain_id, fc::time_point expire = fc::time_point::now() + fc::seconds( 120 ) ) {

   static uint64_t nextid = 0;
   ++nextid;

   account_name creator = config::system_account_name;
   auto priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("nathan")));

   signed_transaction trx;
   trx.expiration = expire;
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             testit{ nextid });
   trx.sign(priv_key, chain_id);

   return std::make_shared<packed_transaction>( std::move(trx), true, packed_transaction::compression_type::none);
}

bool verify_equal( const std::deque<packed_transaction_ptr>& trxs, const std::deque<packed_transaction_ptr>& applied_trxs) {
   for( size_t i = 0; i < trxs.size() && i < applied_trxs.size(); ++i ) {
      if( trxs[i]->id() != applied_trxs[i]->id() ) {
         elog( "[${i}]: ${lhs} != ${rhs}", ("i", i)
               ("lhs", trxs[i]->get_transaction().actions.at(0).name)
               ("rhs", applied_trxs[i]->get_transaction().actions.at(0).name) );
         elog( "[${i}]: ${lhs} != ${rhs}", ("i", i)
               ("lhs", trxs[i]->get_transaction().actions.at(0).data_as<testit>().id)
               ("rhs", applied_trxs[i]->get_transaction().actions.at(0).data_as<testit>().id) );
         return false;
      }
   }
   return true;
}


}

BOOST_AUTO_TEST_SUITE(ordered_trxs_full)

BOOST_AUTO_TEST_CASE(order) {
   boost::filesystem::path temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

   try {
      std::promise<std::tuple<producer_plugin*, chain_plugin*>> plugin_promise;
      std::future<std::tuple<producer_plugin*, chain_plugin*>> plugin_fut = plugin_promise.get_future();
      std::thread app_thread( [&]() {
         std::vector<const char*> argv =
               {"test", "--data-dir", temp.c_str(), "--config-dir", temp.c_str(), "-p", "eosio", "-e" };
         appbase::app().initialize<chain_plugin, producer_plugin>( argv.size(), (char**) &argv[0] );
         appbase::app().startup();
         plugin_promise.set_value(
               {appbase::app().find_plugin<producer_plugin>(), appbase::app().find_plugin<chain_plugin>()} );
         appbase::app().exec();
      } );

      auto[prod_plug, chain_plug] = plugin_fut.get();
      auto chain_id = chain_plug->get_chain_id();

      std::deque<chain::packed_transaction_ptr> applied_trxs;
      auto cc = chain_plug->chain().applied_transaction.connect(
            [&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> x) {
         auto& t = std::get<1>(x);
         if( t->get_transaction().actions.at(0).name != eosio::name("onblock") ) {
            if( !std::get<0>(x)->except ) {
               applied_trxs.push_back( t );
            }
         }
      } );

      named_thread_pool thread_pool( "test", 5 );

      auto queue =
            std::make_shared<fifo_trx_processing_queue<producer_plugin>>( chain_id,
                                                                          config::default_max_variable_signature_length,
                                                                          true,
                                                                          thread_pool.get_executor(),
                                                                          prod_plug,
                                                                          10 );
      queue->run();
      queue->on_block_start();

      std::deque<packed_transaction_ptr> trxs;
      std::atomic<size_t> next_calls = 0;
      std::atomic<bool> next_trx_match = true;
      const size_t num_pushes = 4242;
      for( size_t i = 0; i < num_pushes; ++i ) {
         auto ptrx = make_unique_trx( chain_id );
         trxs.push_back( ptrx );
         queue->push( ptrx,
                      [ptrx, &next_calls, &next_trx_match](
                            const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result ) {
                         if( result.get<chain::transaction_trace_ptr>()->id != ptrx->id() ) {
                            next_trx_match = false;
                         }
                         ++next_calls;
                      } );
    // add handling of abort_block for expected results
//         if( i % (num_pushes / 7) == 0 ) {
//            app().post(priority::high, [](){
//               app().find_plugin<producer_plugin>()->get_integrity_hash();
//            });
//         }
      }

      while( !queue->empty() ) {
         usleep( 100 );
      }

      queue->stop();

      BOOST_CHECK_EQUAL( num_pushes, next_calls );
      BOOST_CHECK( next_trx_match.load() );

      appbase::app().quit();
      queue.reset();
      app_thread.join();

      BOOST_REQUIRE( verify_equal(trxs, applied_trxs ) );

   } catch ( ... ) {
      bfs::remove_all( temp );
      throw;
   }
   bfs::remove_all( temp );
}


BOOST_AUTO_TEST_SUITE_END()
