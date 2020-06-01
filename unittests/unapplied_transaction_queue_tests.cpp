#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/unapplied_transaction_queue.hpp>
#include <eosio/chain/contract_types.hpp>

using namespace eosio;
using namespace eosio::chain;

BOOST_AUTO_TEST_SUITE(unapplied_transaction_queue_tests)

auto unique_trx_meta_data( fc::time_point expire = fc::now() + fc::seconds( 120 ) ) {

   static uint64_t nextid = 0;
   ++nextid;

   signed_transaction trx;
   account_name creator = config::system_account_name;
   trx.expiration = fc::time_point_cast<fc::seconds>( expire );
   trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                             onerror{ nextid, "test", 4 });
   return transaction_metadata::create_no_recover_keys( std::make_shared<packed_transaction>( std::move(trx), true ),
                                                        transaction_metadata::trx_type::input );
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

auto create_test_block_state( deque<transaction_metadata_ptr> trx_metas ) {
   signed_block_ptr block = std::make_shared<signed_block>();
   for( auto& trx_meta : trx_metas ) {
      block->transactions.emplace_back( *trx_meta->packed_trx() );
   }

   block->producer = eosio::chain::config::system_account_name;

   auto priv_key = eosio::testing::base_tester::get_private_key( block->producer, "active" );
   auto pub_key  = eosio::testing::base_tester::get_public_key( block->producer, "active" );

   auto prev = std::make_shared<block_state>();
   auto header_bmroot = digest_type::hash( std::make_pair( block->digest(), prev->blockroot_merkle.get_root() ) );
   auto sig_digest = digest_type::hash( std::make_pair(header_bmroot, prev->pending_schedule.schedule_hash) );
   block->producer_signature = priv_key.sign( sig_digest );

   vector<private_key_type> signing_keys;
   signing_keys.emplace_back( std::move( priv_key ) );

   auto signer = [&]( digest_type d ) {
      std::vector<signature_type> result;
      result.reserve(signing_keys.size());
      for (const auto& k: signing_keys)
         result.emplace_back(k.sign(d));
      return result;
   };
   pending_block_header_state pbhs;
   pbhs.producer = block->producer;
   producer_authority_schedule schedule = { 0, { producer_authority{block->producer, block_signing_authority_v0{ 1, {{pub_key, 1}} } } } };
   pbhs.active_schedule = schedule;
   pbhs.valid_block_signing_authority = block_signing_authority_v0{ 1, {{pub_key, 1}} };
   auto bsp = std::make_shared<block_state>(
         std::move( pbhs ),
         std::move( block ),
         std::move( trx_metas ),
         protocol_feature_set(),
         []( block_timestamp_type timestamp,
             const flat_set<digest_type>& cur_features,
             const vector<digest_type>& new_features )
         {},
         signer
   );

   return bsp;
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
   q.clear_applied( create_test_block_state( { trx1, trx3, trx4 } ) );
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
   auto bs1 = create_test_block_state( { trx1, trx2 } );
   auto bs2 = create_test_block_state( { trx3, trx4, trx5 } );
   auto bs3 = create_test_block_state( { trx6 } );
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
   auto bs4 = create_test_block_state( { trx7 } );
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
   auto bs5 = create_test_block_state( { trx11, trx12, trx13 } );
   auto bs6 = create_test_block_state( { trx11, trx15 } );
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

   auto trx20 = unique_trx_meta_data( fc::now() - fc::seconds( 1 ) );
   auto trx21 = unique_trx_meta_data( fc::now() - fc::seconds( 1 ) );
   auto trx22 = unique_trx_meta_data( fc::now() + fc::seconds( 120 ) );
   auto trx23 = unique_trx_meta_data( fc::now() + fc::seconds( 120 ) );
   q.add_aborted( { trx20, trx22 } );
   q.add_persisted( trx21 );
   q.add_persisted( trx23 );
   q.clear_expired( fc::now(), fc::now() + fc::seconds( 300 ), [](auto, auto){} );
   BOOST_CHECK( q.size() == 2 );
   BOOST_REQUIRE( next( q ) == trx23 );
   BOOST_REQUIRE( next( q ) == trx22 );
   BOOST_CHECK( q.empty() );

   q.add_forked( { bs3, bs2, bs1 } );
   q.add_aborted( { trx9, trx11 } );
   q.add_persisted( trx8 );
   q.clear();
   BOOST_CHECK( q.empty() );
   BOOST_CHECK( q.size() == 0 );
   BOOST_REQUIRE( next( q ) == nullptr );

} FC_LOG_AND_RETHROW() /// unapplied_transaction_queue_test


BOOST_AUTO_TEST_CASE( unapplied_transaction_queue_erase_add ) try {

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

   q.add_incoming( trx1, false, [](auto){} );
   q.add_incoming( trx2, false, [](auto){} );
   q.add_incoming( trx3, false, [](auto){} );
   q.add_incoming( trx4, false, [](auto){} );
   q.add_incoming( trx5, false, [](auto){} );
   q.add_incoming( trx6, false, [](auto){} );

   auto itr = q.incoming_begin();
   auto end = q.incoming_end();

   auto count = q.incoming_size();

   // count required to avoid infinite loop
   while( count && itr != end ) {
      auto trx_meta = itr->trx_meta;
      if( count == 6 ) BOOST_CHECK( trx_meta == trx1 );
      if( count == 5 ) BOOST_CHECK( trx_meta == trx2 );
      if( count == 4 ) BOOST_CHECK( trx_meta == trx3 );
      if( count == 3 ) BOOST_CHECK( trx_meta == trx4 );
      if( count == 2 ) BOOST_CHECK( trx_meta == trx5 );
      if( count == 1 ) BOOST_CHECK( trx_meta == trx6 );
      itr = q.erase( itr );
      q.add_incoming( trx_meta, false, [](auto){} );
      --count;
   }

   BOOST_CHECK( q.size() == 6 );
   BOOST_REQUIRE( next( q ) == trx1 );
   BOOST_REQUIRE( next( q ) == trx2 );
   BOOST_REQUIRE( next( q ) == trx3 );
   BOOST_REQUIRE( next( q ) == trx4 );
   BOOST_REQUIRE( next( q ) == trx5 );
   BOOST_REQUIRE( next( q ) == trx6 );
   BOOST_CHECK( q.empty() );

   // incoming ++itr w/ erase
   q.add_incoming( trx1, false, [](auto){} );
   q.add_incoming( trx2, false, [](auto){} );
   q.add_incoming( trx3, false, [](auto){} );
   q.add_incoming( trx4, false, [](auto){} );
   q.add_incoming( trx5, false, [](auto){} );
   q.add_incoming( trx6, false, [](auto){} );

   itr = q.incoming_begin();
   end = q.incoming_end();

   count = q.incoming_size();
   while( itr != end ) {
      if( count % 2 == 0) {
         itr = q.erase( itr );
      } else {
         ++itr;
      }
      --count;
   }
   BOOST_REQUIRE( count == 0 );
   q.clear();

   // persisted
   q.add_persisted( trx1 );
   q.add_persisted( trx2 );
   q.add_persisted( trx3 );
   q.add_persisted( trx4 );
   q.add_persisted( trx5 );
   q.add_persisted( trx6 );
   q.add_incoming( trx7, false, [](auto){} );

   itr = q.persisted_begin();
   end = q.persisted_end();
   count = q.size() - 1;

   while( count > 0 && itr != end ) {
      auto trx_meta = itr->trx_meta;
      if( count == 6 ) BOOST_CHECK( trx_meta == trx1 );
      if( count == 5 ) BOOST_CHECK( trx_meta == trx2 );
      if( count == 4 ) BOOST_CHECK( trx_meta == trx3 );
      if( count == 3 ) BOOST_CHECK( trx_meta == trx4 );
      if( count == 2 ) BOOST_CHECK( trx_meta == trx5 );
      if( count == 1 ) BOOST_CHECK( trx_meta == trx6 );
      itr = q.erase( itr );
      q.add_persisted( trx_meta );
      --count;
   }
   q.clear();

   q.add_persisted( trx1 );
   q.add_persisted( trx2 );
   q.add_persisted( trx3 );
   q.add_persisted( trx4 );
   q.add_persisted( trx5 );
   q.add_persisted( trx6 );
   q.add_incoming( trx7, false, [](auto){} );

   itr = q.unapplied_begin();
   end = q.unapplied_end();

   count = q.size() - 1;
   while( itr != end ) {
      if( count % 2 == 0) {
         itr = q.erase( itr );
      } else {
         ++itr;
      }
      --count;
   }
   BOOST_REQUIRE( count == 0 );
   q.clear();

} FC_LOG_AND_RETHROW() /// unapplied_transaction_queue_test


BOOST_AUTO_TEST_SUITE_END()
