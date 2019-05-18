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
      intrinsic_debug_log log( log_path );
      log.open();
      log.start_block( 2u );
      log.start_transaction( transaction_id_type() );
      log.start_action( 1ull );
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
      "02" "0100000000000000" // start of action with global sequence number of 1
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

   intrinsic_debug_log log( log_path );
   log.open();
   log.start_block( 2u );
   log.start_transaction( transaction_id_type() );
   log.start_action( 1ull );
   log.acknowledge_intrinsic_without_recording();
   log.record_intrinsic( digest, digest );
   log.finish_block();
   log.start_block( 4u );
   log.start_transaction( transaction_id_type() );
   log.start_transaction( transaction_id_type() );
   log.start_action( 2ull );
   log.record_intrinsic( digest, digest );
   log.record_intrinsic( digest, digest );
   log.start_action( 3ull );
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

BOOST_AUTO_TEST_SUITE_END()
