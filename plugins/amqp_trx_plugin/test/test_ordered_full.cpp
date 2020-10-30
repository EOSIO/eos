#define BOOST_TEST_MODULE ordered_full_exec_trxs
#include <boost/test/included/unit_test.hpp>

#include <eosio/amqp_trx_plugin/fifo_trx_processing_queue.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>

#include <eosio/testing/tester.hpp>

#include <eosio/chain/genesis_state.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/name.hpp>

#include <appbase/application.hpp>

namespace eosio::test::detail {
using namespace eosio::chain::literals;
struct testit {
   uint64_t      id;

   testit( uint64_t id = 0 )
         :id(id){}

   static account_name get_account() {
      return chain::config::system_account_name;
   }

   static action_name get_name() {
      return "testit"_n;
   }
};
}
FC_REFLECT( eosio::test::detail::testit, (id) )

namespace {

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::test::detail;

auto make_unique_trx( const chain_id_type& chain_id ) {

   static uint64_t nextid = 0;
   ++nextid;

   account_name creator = config::system_account_name;

   signed_transaction trx;
   // if a transaction expires after it was aborted then it will not be included in a block
   trx.expiration = fc::time_point::now() + fc::seconds( nextid % 20 == 0 ? 0 : 60 ); // fail some transactions via expired
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             testit{ nextid });
   if( nextid % 13 == 0 ) {
      auto bad_priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("kevin")));
      trx.sign( bad_priv_key, chain_id );
   } else {
      auto priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("nathan")));
      trx.sign( priv_key, chain_id );
   }

   return std::make_shared<packed_transaction>( std::move(trx), true, packed_transaction::compression_type::none);
}

// verify all trxs are in blocks in order
bool verify_equal( const std::deque<packed_transaction_ptr>& trxs, const std::deque<block_state_ptr>& all_blocks) {
   size_t num_trxs = 0;
   for( size_t i = 0, j = 0, k = 0; i < trxs.size() && j < all_blocks.size(); ++i ) {

      auto next_trx = [&]() -> const transaction& {
         while( j < all_blocks.size() ) {
            const auto& bs = all_blocks[j];
            if( k < bs->block->transactions.size() ) {
               const auto& trx = std::get<packed_transaction>(bs->block->transactions[k].trx).get_transaction();
               ++k;
               return trx;
            } else {
               k = 0;
               ++j;
            }
         }
         BOOST_REQUIRE_MESSAGE( false, "Hit end of all_blocks" );
         return std::get<packed_transaction>(all_blocks.at(j)->block->transactions.at(0).trx).get_transaction(); // avoid warning
      };

      const auto& trx = next_trx();

      if( trxs[i]->id() != trx.id() ) {
         elog( "[${i}],[${j},${k}]: ${lhs} != ${rhs}", ("i", i)("j", j)("k", k)
               ("lhs", trxs[i]->get_transaction().actions.at(0).data_as<testit>().id)
               ("rhs", trx.actions.at(0).data_as<testit>().id) );
         elog( "[${i}],[${j},${k}]: ${lhs} != ${rhs}", ("i", i)("j", j)("k", k)
               ("lhs", trxs[i]->id())
               ("rhs", trx.id()) );
         return false;
      } else {
         ++num_trxs;
      }
   }
   BOOST_CHECK_EQUAL( num_trxs, trxs.size() );
   return true;
}


}

BOOST_AUTO_TEST_SUITE(ordered_trxs_full)

// Integration test of fifo_trx_processing_queue and producer_plugin
// Test verifies that transactions are processed in the order they are submitted to the fifo_trx_processing_queue
// even when blocks are aborted and some transactions fail.
BOOST_AUTO_TEST_CASE(order) {
   boost::filesystem::path temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();

   try {
      std::promise<std::tuple<producer_plugin*, chain_plugin*>> plugin_promise;
      std::future<std::tuple<producer_plugin*, chain_plugin*>> plugin_fut = plugin_promise.get_future();
      std::thread app_thread( [&]() {
         fc::logger::get(DEFAULT_LOGGER).set_log_level(fc::log_level::debug);
         std::vector<const char*> argv =
               {"test", "--data-dir", temp.c_str(), "--config-dir", temp.c_str(),
                "-p", "eosio", "-e", "--max-transaction-time", "500" };
         appbase::app().initialize<chain_plugin, producer_plugin>( argv.size(), (char**) &argv[0] );
         appbase::app().startup();
         plugin_promise.set_value(
               {appbase::app().find_plugin<producer_plugin>(), appbase::app().find_plugin<chain_plugin>()} );
         appbase::app().exec();
      } );

      auto[prod_plug, chain_plug] = plugin_fut.get();
      auto chain_id = chain_plug->get_chain_id();

      named_thread_pool thread_pool( "test", 5 );

      auto queue =
            std::make_shared<fifo_trx_processing_queue<producer_plugin>>( chain_id,
                                                                          config::default_max_variable_signature_length,
                                                                          true,
                                                                          thread_pool.get_executor(),
                                                                          prod_plug,
                                                                          10 );

      std::deque<block_state_ptr> all_blocks;
      std::promise<void> empty_blocks_promise;
      std::future<void> empty_blocks_fut = empty_blocks_promise.get_future();
      std::atomic<size_t> num_aborts = 0;
      auto ab = chain_plug->chain().accepted_block.connect( [&](const block_state_ptr& bsp) {
         static int num_empty = std::numeric_limits<int>::max();
         all_blocks.push_back( bsp );
         if( bsp->block->transactions.empty() ) {
            --num_empty;
            if( num_empty == 0 ) empty_blocks_promise.set_value();
         } else { // we want a couple of empty blocks after we have some non-empty blocks
            num_empty = 2;
         }
         queue->on_block_stop();
      } );
      auto ba = chain_plug->chain().block_abort.connect( [&]( uint32_t bn ) {
         ++num_aborts;
         queue->on_block_stop();
      } );
      auto bs = chain_plug->chain().block_start.connect( [&]( uint32_t bn ) {
         queue->on_block_start();
      } );

      queue->run();

      std::deque<packed_transaction_ptr> trxs;
      std::atomic<size_t> next_calls = 0;
      std::atomic<bool> next_trx_match = true;
      const size_t num_pushes = 4242;
      for( size_t i = 1; i <= num_pushes; ++i ) {
         auto ptrx = make_unique_trx( chain_id );
         queue->push( ptrx,
                      [ptrx, &next_calls, &next_trx_match, &trxs](
                            const std::variant<fc::exception_ptr, chain::transaction_trace_ptr>& result ) {
                         if( !std::holds_alternative<fc::exception_ptr>(result) && !std::get<chain::transaction_trace_ptr>(result)->except ) {
                            if( std::get<chain::transaction_trace_ptr>(result)->id == ptrx->id() ) {
                               trxs.push_back( ptrx );
                            } else {
                               next_trx_match = false;
                            }
                         }
                         ++next_calls;
                      } );
         if( i % (num_pushes/3) == 0 ) {
            // need to sleep or a fast machine might put all trxs in one block
            usleep( config::block_interval_us / 2 );
         }
         if( i % 500 == 0 && num_aborts <= 3 ) {
            // get_integrity_hash aborts block and places aborted trxs into unapplied_transaction_queue
            // verifying that aborting block does not change order of transactions when re-applied
            app().post(priority::high, [](){
               app().find_plugin<producer_plugin>()->get_integrity_hash();
            });
         }
      }

      empty_blocks_fut.wait_for(std::chrono::seconds(10));

      queue->stop();

      BOOST_CHECK( all_blocks.size() > 3 ); // should have a few blocks otherwise test is running too fast
      BOOST_CHECK( num_aborts > 3 ); // should have a few abort_blocks or maybe get_integrity_hash has changed
      BOOST_CHECK_EQUAL( num_pushes, next_calls );
      BOOST_CHECK( next_trx_match.load() );

      appbase::app().quit();
      app_thread.join();
      queue.reset();

      BOOST_REQUIRE( verify_equal(trxs, all_blocks ) );

   } catch ( ... ) {
      bfs::remove_all( temp );
      throw;
   }
   bfs::remove_all( temp );
}


BOOST_AUTO_TEST_SUITE_END()
