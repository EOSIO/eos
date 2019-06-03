/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/test/unit_test.hpp>
#include <eosio/producer_plugin/unapplied_transaction_queue.hpp>
#include <eosio/chain/contract_types.hpp>

using namespace eosio;
using namespace eosio::chain;

BOOST_AUTO_TEST_SUITE(producer_plugin_tests)

auto unique_trx_meta_data() {

   static uint64_t nextid = 0;
   ++nextid;

   signed_transaction trx;
   account_name creator = config::system_account_name;
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             onerror{ nextid, "test", 4 });
   return std::make_shared<transaction_metadata>( trx );
}

BOOST_AUTO_TEST_CASE( unapplied_transaction_queue_test ) try {

   unapplied_transaction_queue q;

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
   auto p = q.next();
   BOOST_CHECK( p == nullptr );

   // 1 subjective failure
   q.add_subjective_failure( trx1 );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == nullptr );
   BOOST_REQUIRE( q.next() == nullptr );

   // fifo subjective
   q.add_subjective_failure( trx1 );
   q.add_subjective_failure( trx2 );
   q.add_subjective_failure( trx3 );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE( q.next() == trx3 );
   BOOST_REQUIRE( q.next() == nullptr );

   // fifo aborted
   q.add_aborted( { trx1, trx2, trx3 } );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE( q.next() == trx3 );
   BOOST_REQUIRE( q.next() == nullptr );

   // order: aborted, subjective
   q.add_subjective_failure( trx6 );
   q.add_aborted( { trx1, trx2, trx3 } );
   q.add_aborted( { trx4, trx5 } );
   q.add_subjective_failure( trx7 );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE( q.next() == trx3 );
   BOOST_REQUIRE( q.next() == trx4 );
   BOOST_REQUIRE( q.next() == trx5 );
   BOOST_REQUIRE( q.next() == trx6 );
   BOOST_REQUIRE( q.next() == trx7 );
   BOOST_REQUIRE( q.next() == nullptr );

   // fifo forked, one fork
   auto bs1 = std::make_shared<block_state>();
   bs1->trxs = { trx1, trx2 };
   auto bs2 = std::make_shared<block_state>();
   bs2->trxs = { trx3, trx4, trx5 };
   auto bs3 = std::make_shared<block_state>();
   bs3->trxs = { trx6 };
   q.add_forked( { bs3, bs2, bs1 } );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE_EQUAL( q.next(), trx3 );
   BOOST_REQUIRE( q.next() == trx4 );
   BOOST_REQUIRE( q.next() == trx5 );
   BOOST_REQUIRE( q.next() == trx6 );
   BOOST_REQUIRE( q.next() == nullptr );

   // fifo forked, multi forks
   auto bs4 = std::make_shared<block_state>();
   bs4->trxs = { trx7 };
   q.add_forked( { bs3, bs2, bs1 } );
   q.add_forked( { bs4 } );
   q.add_forked( { bs3, bs2 } );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE_EQUAL( q.next(), trx3 );
   BOOST_REQUIRE( q.next() == trx4 );
   BOOST_REQUIRE( q.next() == trx5 );
   BOOST_REQUIRE( q.next() == trx6 );
   BOOST_REQUIRE( q.next() == trx7 );
   BOOST_REQUIRE_EQUAL( q.next(), trx3 );
   BOOST_REQUIRE( q.next() == trx4 );
   BOOST_REQUIRE( q.next() == trx5 );
   BOOST_REQUIRE( q.next() == trx6 );
   BOOST_REQUIRE( q.next() == nullptr );

   // fifo forked
   q.add_forked( { bs1 } );
   q.add_forked( { bs3, bs2 } );
   q.add_forked( { bs4 } );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE_EQUAL( q.next(), trx3 );
   BOOST_REQUIRE( q.next() == trx4 );
   BOOST_REQUIRE( q.next() == trx5 );
   BOOST_REQUIRE( q.next() == trx6 );
   BOOST_REQUIRE( q.next() == trx7 );
   BOOST_REQUIRE( q.next() == nullptr );

   auto trx11 = unique_trx_meta_data();
   auto trx12 = unique_trx_meta_data();
   auto trx13 = unique_trx_meta_data();
   auto trx14 = unique_trx_meta_data();
   auto trx15 = unique_trx_meta_data();
   auto trx16 = unique_trx_meta_data();
   auto trx17 = unique_trx_meta_data();
   auto trx18 = unique_trx_meta_data();
   auto trx19 = unique_trx_meta_data();

   // altogether, order fifo: aborted, subjectively failed, forked
   q.add_forked( { bs3, bs2, bs1 } );
   q.add_subjective_failure( trx7 );
   q.add_aborted( { trx9, trx11 } );
   q.add_subjective_failure( trx8 );
   q.add_aborted( { trx12, trx13 } );
   q.add_forked( { bs4, bs3, bs2, bs1 } );
   BOOST_REQUIRE( q.next() == trx9 );
   BOOST_REQUIRE( q.next() == trx11 );
   BOOST_REQUIRE( q.next() == trx12 );
   BOOST_REQUIRE( q.next() == trx13 );
   BOOST_REQUIRE( q.next() == trx7 );
   BOOST_REQUIRE( q.next() == trx8 );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE_EQUAL( q.next(), trx3 );
   BOOST_REQUIRE( q.next() == trx4 );
   BOOST_REQUIRE( q.next() == trx5 );
   BOOST_REQUIRE( q.next() == trx6 );
   BOOST_REQUIRE( q.next() == trx1 );
   BOOST_REQUIRE( q.next() == trx2 );
   BOOST_REQUIRE_EQUAL( q.next(), trx3 );
   BOOST_REQUIRE( q.next() == trx4 );
   BOOST_REQUIRE( q.next() == trx5 );
   BOOST_REQUIRE( q.next() == trx6 );
   BOOST_REQUIRE( q.next() == trx7 );
   BOOST_REQUIRE( q.next() == nullptr );



} FC_LOG_AND_RETHROW() /// unapplied_transaction_queue_test


BOOST_AUTO_TEST_SUITE_END()
