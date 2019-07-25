/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain/intrinsic_debug_log.hpp>

#include <fc/filesystem.hpp>
#include <fc/io/json.hpp>
#include <fstream>
#include <boost/test/unit_test.hpp>

using namespace eosio;
using namespace chain;

BOOST_AUTO_TEST_SUITE(intrinsic_debug_log_tests)

BOOST_AUTO_TEST_CASE(basic_test) {
   fc::temp_directory  tempdir;
   auto log_path = tempdir.path() / "intrinsic.log";
   {
      auto digest = fc::variant("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20").as<digest_type>();
      name alice("alice");
      name foo("foo");
      intrinsic_debug_log log( log_path );
      log.open();
      log.start_block( 2u );
      log.start_transaction( transaction_id_type() );
      log.start_action( 1ull, alice, alice, foo );
      log.acknowledge_intrinsic_without_recording();
      log.record_intrinsic( digest, digest );
      log.finish_block();
      log.close();
   }

   std::fstream log_stream( log_path.generic_string().c_str(), std::ios::in | std::ios::binary );
   log_stream.seekg( 0, std::ios::end );
   std::size_t file_size = log_stream.tellg();
   log_stream.seekg( 0, std::ios::beg );

   vector<char> buffer( file_size );
   log_stream.read( buffer.data(), file_size );
   log_stream.close();

   const char* expected =
      "00" "02000000" // start of block 2
      "01" "0000000000000000000000000000000000000000000000000000000000000000" // start of transaction
      "02"            // start of action with:
      "0100000000000000"  // global sequence number of 1,
      "0000000000855c34"  // receiver "alice",
      "0000000000855c34"  // first_receiver "alice",
      "000000000000285d"  // and action_name "foo".
      "03"                                                            // recording an intrinsic with:
      "01000000"                                                          // an ordinal of 1,
      "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"  // this arguments hash,
      "0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20"  // and this WASM lineary memory hash.
      "00" "0000000000000000" // end of the block with pointer to position of the start of the block
   ;

   BOOST_CHECK_EQUAL( fc::to_hex( buffer ), expected );
}

BOOST_AUTO_TEST_CASE(iterate_test) {
   fc::temp_directory  tempdir;
   auto log_path = tempdir.path() / "intrinsic.log";
   auto digest = fc::variant("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20").as<digest_type>();
   name alice("alice");
   name bob("bob");
   name foo("foo");
   name bar("bar");

   intrinsic_debug_log log( log_path );
   log.open();
   log.close();

   log.open( intrinsic_debug_log::open_mode::read_only );
   BOOST_CHECK( log.begin_block() == log.end_block() );
   log.close();

   log.open();
   log.start_block( 2u );
   log.start_transaction( transaction_id_type() );
   log.start_action( 1ull, alice, alice, foo );
   log.acknowledge_intrinsic_without_recording();
   log.record_intrinsic( digest, digest );
   log.finish_block();
   log.close();

   log.open( intrinsic_debug_log::open_mode::read_only );
   BOOST_CHECK( log.begin_block() != log.end_block() );
   log.close();

   log.open();
   log.start_block( 4u );
   log.start_transaction( transaction_id_type() );
   log.start_transaction( transaction_id_type() );
   log.start_action( 2ull, alice, alice, bar );
   log.record_intrinsic( digest, digest );
   log.record_intrinsic( digest, digest );
   log.start_action( 3ull, bob, alice, bar );
   log.finish_block();
   log.close();

   log.open( intrinsic_debug_log::open_mode::read_only );
   std::vector< std::pair< uint32_t, uint32_t > > expected_blocks = {
      {2u, 1u},
      {4u, 2u}
   };

   {
      auto block_itr = log.begin_block();
      const auto block_end_itr = log.end_block();
      for( auto itr = expected_blocks.begin(); block_itr != block_end_itr; ++block_itr, ++itr ) {
         BOOST_REQUIRE( itr != expected_blocks.end() );
         BOOST_CHECK_EQUAL( block_itr->block_num, itr->first );
         BOOST_CHECK_EQUAL( block_itr->transactions.size(), itr->second );
         wdump( (fc::json::to_pretty_string(*block_itr)) );
      }

      for( auto itr = expected_blocks.rbegin(); itr != expected_blocks.rend(); ++itr ) {
         --block_itr;
         BOOST_CHECK_EQUAL( block_itr->block_num, itr->first );
         BOOST_CHECK_EQUAL( block_itr->transactions.size(), itr->second );
      }
      log.close(); // invalidates block_itr;
   }

   log.open( intrinsic_debug_log::open_mode::read_only );
   {
      auto block_itr = log.end_block();
      --block_itr;
      auto itr = expected_blocks.rbegin();
      for( ; itr != expected_blocks.rend() && itr->first > 3u; ++itr ) {
         --block_itr;
         // Avoid dereferencing block_itr to not read block and add to cache.
      }

      BOOST_REQUIRE( itr != expected_blocks.rend() );
      BOOST_REQUIRE_EQUAL( itr->first, 2u ); // ensure we are at the expected block

      // block_itr should also be point to a valid block (the same one with a block height of 2).
      // However, it hasn't been cached yet since we have not yet dereferenced it.

      BOOST_REQUIRE_EQUAL( block_itr->block_num, 2u );
   }
}

BOOST_AUTO_TEST_CASE(difference_test) {
   fc::temp_directory  tempdir;
   auto log1_path = tempdir.path() / "intrinsic1.log";
   auto log2_path = tempdir.path() / "intrinsic2.log";
   auto trx_id = fc::variant("0102030405060708090a0b0c0d0e0f100102030405060708090a0b0c0d0e0f10").as<transaction_id_type>();
   auto digest = fc::variant("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20").as<digest_type>();
   name alice("alice");
   name bob("bob");
   name foo("foo");
   name bar("bar");

   intrinsic_debug_log log1( log1_path );
   {
      log1.open();
      log1.start_block( 2u );
      log1.start_transaction( transaction_id_type() );
      log1.start_action( 1ull, alice, alice, foo );
      log1.acknowledge_intrinsic_without_recording();
      log1.record_intrinsic( digest, digest );
      log1.finish_block();
      log1.start_block( 4u );
      log1.start_transaction( trx_id );
      log1.start_action( 2ull, alice, alice, bar );
      log1.record_intrinsic( digest, digest );
      log1.record_intrinsic( digest, digest );
      log1.start_action( 3ull, bob, alice, bar );
      log1.finish_block();
      log1.close();
   }

   intrinsic_debug_log log2( log2_path );
   {
      log2.open();
      log2.start_block( 2u );
      log2.start_transaction( transaction_id_type() );
      log2.start_action( 1ull, alice, alice, foo );
      log2.acknowledge_intrinsic_without_recording();
      log2.record_intrinsic( digest, digest );
      log2.finish_block();
      log2.start_block( 4u );
      log2.start_transaction( trx_id );
      log2.start_action( 2ull, alice, alice, bar );
      log2.record_intrinsic( digest, digest );
      log2.start_action( 3ull, bob, alice, bar );
      log2.finish_block();
      log2.close();
   }

   auto result = intrinsic_debug_log::find_first_difference( log1, log2 );
   BOOST_REQUIRE( result.has_value() );

   wdump( (fc::json::to_pretty_string(*result)) );

   BOOST_CHECK_EQUAL( result->block_num, 4u );
   BOOST_CHECK_EQUAL( result->trx_id, trx_id );
   BOOST_CHECK_EQUAL( result->global_sequence_num, 2ull );
   BOOST_CHECK_EQUAL( result->receiver.to_string(), "alice" );
   BOOST_CHECK_EQUAL( result->first_receiver.to_string(), "alice" );
   BOOST_CHECK_EQUAL( result->action_name.to_string(), "bar" );
   BOOST_CHECK_EQUAL( result->lhs_recorded_intrinsics.size(), 2 );
   BOOST_CHECK_EQUAL( result->rhs_recorded_intrinsics.size(), 1 );
}

BOOST_AUTO_TEST_CASE(equivalence_test) {
   fc::temp_directory  tempdir;
   auto log1_path = tempdir.path() / "intrinsic1.log";
   auto log2_path = tempdir.path() / "intrinsic2.log";
   auto trx_id = fc::variant("0102030405060708090a0b0c0d0e0f100102030405060708090a0b0c0d0e0f10").as<transaction_id_type>();
   auto digest = fc::variant("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20").as<digest_type>();
   name alice("alice");
   name bob("bob");
   name foo("foo");
   name bar("bar");

   intrinsic_debug_log log1( log1_path );
   {
      log1.open();
      log1.start_block( 1u );
      log1.finish_block();
      log1.start_block( 2u );
      log1.start_transaction( transaction_id_type() );
      log1.abort_transaction();
      log1.start_transaction( transaction_id_type() );
      log1.start_action( 1ull, alice, alice, foo );
      log1.acknowledge_intrinsic_without_recording();
      log1.record_intrinsic( digest, digest );
      log1.finish_block();
      log1.start_block( 4u );
      log1.start_transaction( trx_id );
      log1.start_action( 2ull, alice, alice, bar );
      log1.record_intrinsic( digest, digest );
      log1.start_action( 3ull, bob, alice, bar );
      log1.finish_block();
      log1.start_block( 5u );
      log1.start_transaction( transaction_id_type() );
      log1.start_action( 4ull, alice, alice, foo );
      log1.abort_block();
      log1.start_block( 5u );
      log1.close();
   }

   intrinsic_debug_log log2( log2_path );
   {
      log2.open();
      log2.start_block( 2u );
      log2.start_transaction( transaction_id_type() );
      log2.start_action( 1ull, alice, alice, foo );
      log2.acknowledge_intrinsic_without_recording();
      log2.record_intrinsic( digest, digest );
      log2.finish_block();
      log2.start_block( 4u );
      log2.start_transaction( trx_id );
      log2.start_action( 2ull, alice, alice, bar );
      log2.record_intrinsic( digest, digest );
      log2.start_action( 3ull, bob, alice, bar );
      log2.finish_block();
      log2.close();
   }

   auto result = intrinsic_debug_log::find_first_difference( log1, log2 );
   if( result ) {
      wdump( (fc::json::to_pretty_string(*result)) );
   }
   BOOST_REQUIRE( !result );
}

BOOST_AUTO_TEST_CASE(auto_finish_block_test) {
   fc::temp_directory  tempdir;
   auto ref_log1_path = tempdir.path() / "intrinsic1.log";
   auto ref_log2_path = tempdir.path() / "intrinsic2.log";
   auto log_path      = tempdir.path() / "intrinsic.log";
   auto trx_id = fc::variant("0102030405060708090a0b0c0d0e0f100102030405060708090a0b0c0d0e0f10").as<transaction_id_type>();
   auto digest = fc::variant("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20").as<digest_type>();
   name alice("alice");
   name foo("foo");

   intrinsic_debug_log ref_log1( ref_log1_path ); // reference log 1
   {
      ref_log1.open();
      ref_log1.start_block( 1u );
      ref_log1.finish_block();
      ref_log1.close();
   }

   intrinsic_debug_log ref_log2( ref_log2_path ); // reference log 2
   {
      ref_log2.open();
      ref_log2.start_block( 1u );
      ref_log2.finish_block();
      ref_log2.start_block( 2u );
      ref_log2.start_transaction( trx_id );
      ref_log2.start_action( 1ull, alice, alice, foo );
      ref_log2.acknowledge_intrinsic_without_recording();
      ref_log2.record_intrinsic( digest, digest );
      ref_log2.finish_block();
      ref_log2.close();
   }

   // Mode 1 (default mode): do not automatically finish any pending block.
   // This mode enforces that the log remains consistent with block activity (truncate blocks if necessary).
   {
      {
         intrinsic_debug_log log( log_path );
         log.open();
         log.start_block( 1u );
         log.finish_block();
         log.start_block( 2u );
         log.start_transaction( trx_id );
         log.start_action( 1ull, alice, alice, foo );
         log.acknowledge_intrinsic_without_recording();
         log.record_intrinsic( digest, digest );
      }

      intrinsic_debug_log log( log_path );
      log.open( intrinsic_debug_log::open_mode::read_only );

      auto result = intrinsic_debug_log::find_first_difference( log, ref_log1 );
      BOOST_REQUIRE( !result );
   }

   // Mode 2: automatically finish any pending block.
   // This mode keeps recent information in log during any shutdown by automatically finish the block even if it
   // is not explicitly called. This may result in the data recorded for last block in the log to not have all
   // the data that actually occurred in that block.
   {
      {
         intrinsic_debug_log log( log_path );
         log.open( intrinsic_debug_log::open_mode::continue_existing_and_auto_finish_block );
         log.start_block( 1u );
         log.finish_block();
         log.start_block( 2u );
         log.start_transaction( trx_id);
         log.start_action( 1ull, alice, alice, foo );
         log.acknowledge_intrinsic_without_recording();
         log.record_intrinsic( digest, digest );
      }

      intrinsic_debug_log log( log_path );
      log.open( intrinsic_debug_log::open_mode::read_only );

      auto result = intrinsic_debug_log::find_first_difference( log, ref_log2 );
      BOOST_REQUIRE( !result );
   }
}

BOOST_AUTO_TEST_SUITE_END()
