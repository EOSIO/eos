/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>
#include <eosio/chain/unapplied_transaction_queue.hpp>
#include <eosio/chain/contract_types.hpp>

using namespace eosio;
using namespace eosio::chain;

BOOST_AUTO_TEST_SUITE(unapplied_transaction_queue_tests)

auto unique_trx_meta_data() {

   static uint64_t nextid = 0;
   ++nextid;

   signed_transaction trx;
   account_name creator = config::system_account_name;
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             onerror{ nextid, "test", 4 });
   return std::make_shared<transaction_metadata>( trx );
}

auto next( unapplied_transaction_queue& q ) {
   transaction_metadata_ptr trx;
   auto itr = q.begin();
   if( itr != q.end() ) {
      trx = itr->trx_meta;
      q.erase( itr );
   }
   return trx;
}

BOOST_AUTO_TEST_CASE( unapplied_transaction_queue_test ) try {

   unapplied_transaction_queue q;
   BOOST_CHECK( q.empty() );
   BOOST_CHECK( q.size() == 0 );

   auto trx1 = unique_trx_meta_data();
   auto trx2 = unique_trx_meta_data();
   auto trx3 = unique_trx_meta_data();
   auto trx4 = unique_trx_meta_data();
   auto trx5 = unique_trx_meta_data();
   auto trx6 = unique_trx_meta_data();
   auto trx7 = unique_trx_meta_data();
   auto trx8 = unique_trx_meta_data();
   auto trx9 = unique_trx_meta_data();

   // empty
   auto p = next( q );
   BOOST_CHECK( p == nullptr );

   // 1 persisted
   q.add_persisted( trx1 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   // fifo persisted
   q.add_persisted( trx1 );
   q.add_persisted( trx2 );
   q.add_persisted( trx3 );
   BOOST_CHECK( q.size() == 3 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx3 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   // fifo aborted
   q.add_aborted( { trx1, trx2, trx3 } );
   q.add_aborted( { trx1, trx2, trx3 } ); // duplicates ignored
   BOOST_CHECK( q.size() == 3 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx3 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   // clear applied
   q.add_aborted( { trx1, trx2, trx3 } );
   q.clear_applied( { trx1, trx3, trx4 } );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   // order: persisted, aborted
   q.add_persisted( trx6 );
   q.add_aborted( { trx1, trx2, trx3 } );
   q.add_aborted( { trx4, trx5 } );
   q.add_persisted( trx7 );
   BOOST_CHECK( q.size() == 7 );
   BOOST_REQUIRE( next( q ) == trx6 );
   BOOST_CHECK( q.size() == 6 );
   BOOST_REQUIRE( next( q ) == trx7 );
   BOOST_CHECK( q.size() == 5 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 4 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 3 );
   BOOST_REQUIRE( next( q ) == trx3 );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx4 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx5 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   // fifo forked, one fork
   auto bs1 = std::make_shared<block_state>();
   bs1->trxs = { trx1, trx2 };
   auto bs2 = std::make_shared<block_state>();
   bs2->trxs = { trx3, trx4, trx5 };
   auto bs3 = std::make_shared<block_state>();
   bs3->trxs = { trx6 };
   q.add_forked( { bs3, bs2, bs1, bs1 } ); // bs1 duplicate ignored
   BOOST_CHECK( q.size() == 6 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 5 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 4 );
   BOOST_REQUIRE_EQUAL( next( q ), trx3 );
   BOOST_CHECK( q.size() == 3 );
   BOOST_REQUIRE( next( q ) == trx4 );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx5 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx6 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   // fifo forked
   auto bs4 = std::make_shared<block_state>();
   bs4->trxs = { trx7 };
   q.add_forked( { bs1 } );
   q.add_forked( { bs3, bs2 } );
   q.add_forked( { bs4 } );
   BOOST_CHECK( q.size() == 7 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 6 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 5 );
   BOOST_REQUIRE_EQUAL( next( q ), trx3 );
   BOOST_CHECK( q.size() == 4 );
   BOOST_REQUIRE( next( q ) == trx4 );
   BOOST_CHECK( q.size() == 3 );
   BOOST_REQUIRE( next( q ) == trx5 );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx6 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx7 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   auto trx11 = unique_trx_meta_data();
   auto trx12 = unique_trx_meta_data();
   auto trx13 = unique_trx_meta_data();
   auto trx14 = unique_trx_meta_data();
   auto trx15 = unique_trx_meta_data();
   auto trx16 = unique_trx_meta_data();
   auto trx17 = unique_trx_meta_data();
   auto trx18 = unique_trx_meta_data();
   auto trx19 = unique_trx_meta_data();

   // fifo forked, multi forks
   auto bs5 = std::make_shared<block_state>();
   auto bs6 = std::make_shared<block_state>();
   bs5->trxs = { trx11, trx12, trx13 };
   bs6->trxs = { trx11, trx15 };
   q.add_forked( { bs3, bs2, bs1 } );
   q.add_forked( { bs4 } );
   q.add_forked( { bs3, bs2 } ); // dups ignored
   q.add_forked( { bs6, bs5 } );
   BOOST_CHECK_EQUAL( q.size(), 11 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 10 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 9 );
   BOOST_REQUIRE_EQUAL( next( q ), trx3 );
   BOOST_CHECK( q.size() == 8 );
   BOOST_REQUIRE( next( q ) == trx4 );
   BOOST_CHECK( q.size() == 7 );
   BOOST_REQUIRE( next( q ) == trx5 );
   BOOST_CHECK( q.size() == 6 );
   BOOST_REQUIRE( next( q ) == trx6 );
   BOOST_CHECK( q.size() == 5 );
   BOOST_REQUIRE( next( q ) == trx7 );
   BOOST_CHECK( q.size() == 4 );
   BOOST_REQUIRE_EQUAL( next( q ), trx11 );
   BOOST_CHECK( q.size() == 3 );
   BOOST_REQUIRE( next( q ) == trx12 );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx13 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx15 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   // altogether, order fifo: persisted, forked, aborted
   q.add_forked( { bs3, bs2, bs1 } );
   q.add_persisted( trx16 );
   q.add_aborted( { trx9, trx14 } );
   q.add_persisted( trx8 );
   q.add_aborted( { trx18, trx19 } );
   q.add_forked( { bs6, bs5, bs4 } );
   BOOST_CHECK( q.size() == 17 );
   BOOST_REQUIRE( next( q ) == trx16 );
   BOOST_CHECK( q.size() == 16 );
   BOOST_REQUIRE( next( q ) == trx8 );
   BOOST_CHECK( q.size() == 15 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_CHECK( q.size() == 14 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_CHECK( q.size() == 13 );
   BOOST_REQUIRE_EQUAL( next( q ), trx3 );
   BOOST_CHECK( q.size() == 12 );
   BOOST_REQUIRE( next( q ) == trx4 );
   BOOST_CHECK( q.size() == 11 );
   BOOST_REQUIRE( next( q ) == trx5 );
   BOOST_CHECK( q.size() == 10 );
   BOOST_REQUIRE( next( q ) == trx6 );
   BOOST_CHECK( q.size() == 9 );
   BOOST_REQUIRE( next( q ) == trx7 );
   BOOST_CHECK( q.size() == 8 );
   BOOST_REQUIRE( next( q ) == trx11 );
   BOOST_CHECK( q.size() == 7 );
   BOOST_REQUIRE_EQUAL( next( q ), trx12 );
   BOOST_CHECK( q.size() == 6 );
   BOOST_REQUIRE( next( q ) == trx13 );
   BOOST_CHECK( q.size() == 5 );
   BOOST_REQUIRE( next( q ) == trx15 );
   BOOST_CHECK( q.size() == 4 );
   BOOST_REQUIRE( next( q ) == trx9 );
   BOOST_CHECK( q.size() == 3 );
   BOOST_REQUIRE( next( q ) == trx14 );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx18 );
   BOOST_CHECK( q.size() == 1 );
   BOOST_REQUIRE( next( q ) == trx19 );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );
   BOOST_CHECK( q.empty() );

   q.add_forked( { bs3, bs2, bs1 } );
   q.add_aborted( { trx9, trx11 } );
   q.add_persisted( trx8 );
   q.clear();
   BOOST_CHECK( q.empty() );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );

} FC_LOG_AND_RETHROW() /// unapplied_transaction_queue_test


BOOST_AUTO_TEST_SUITE_END()
