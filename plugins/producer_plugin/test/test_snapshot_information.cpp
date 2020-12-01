#define BOOST_TEST_MODULE snapshot_information
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/chain/snapshot.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/testing/snapshot_suites.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/producer_plugin/pending_snapshot.hpp>
#include <contracts.hpp>
#include <snapshots.hpp>

using namespace eosio;
using namespace eosio::testing;
using namespace boost::system;

namespace {
    eosio::producer_plugin::snapshot_information test_snap_info;
}

BOOST_AUTO_TEST_SUITE(snapshot_tests)

using next_t = eosio::producer_plugin::next_function<eosio::producer_plugin::snapshot_information>;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_snapshot_information, SNAPSHOT_SUITE, snapshot_suites) {
   tester chain;
   const chainbase::bfs::path parent_path = chain.get_config().blog.log_dir.parent_path();

   chain.create_account("snapshot"_n);
   chain.produce_blocks(1);
   chain.set_code("snapshot"_n, contracts::snapshot_test_wasm());
   chain.set_abi("snapshot"_n, contracts::snapshot_test_abi().data());
   chain.produce_blocks(1);

   auto block = chain.produce_block();
   BOOST_REQUIRE_EQUAL(block->block_num(), 6); // ensure that test setup stays consistent with original snapshot setup
   // undo the auto-pending from tester
   chain.control->abort_block();

   auto block2 = chain.produce_block();
   BOOST_REQUIRE_EQUAL(block2->block_num(), 7); // ensure that test setup stays consistent with original snapshot setup
   // undo the auto-pending from tester
   chain.control->abort_block();

   // write snapshot
   auto write_snapshot = [&]( const bfs::path& p ) -> void {
      if ( !bfs::exists( p.parent_path() ) )
         bfs::create_directory( p.parent_path() );

      // create the snapshot
      auto snap_out = std::ofstream(p.generic_string(), (std::ios::out | std::ios::binary));
      auto writer = std::make_shared<ostream_snapshot_writer>(snap_out);
      (*chain.control).write_snapshot(writer);
      writer->finalize();
      snap_out.flush();
      snap_out.close();
   };

   auto final_path = eosio::pending_snapshot::get_final_path(block2->previous, "../snapshots/");
   auto pending_path = eosio::pending_snapshot::get_pending_path(block2->previous, "../snapshots/");

   write_snapshot( pending_path );
   next_t next;
   eosio::pending_snapshot pending{ block2->previous, next, pending_path.generic_string(), final_path.generic_string(), nullptr };
   test_snap_info = pending.finalize(*chain.control);
   BOOST_REQUIRE_EQUAL(test_snap_info.head_block_num, 6);
   BOOST_REQUIRE_EQUAL(test_snap_info.version, chain_snapshot_header::current_version);
}

BOOST_AUTO_TEST_SUITE_END()
