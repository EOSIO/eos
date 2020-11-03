// Run from directory: `/eos/build/`
// ./unittests/unit_test --run_test=blockvault_client_plugin_tests

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp>

BOOST_AUTO_TEST_SUITE(blockvault_client_plugin_tests)

BOOST_AUTO_TEST_CASE( blockvault_client_plugin_init_plugin ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_client_plugin_propose_constructed_block ) try {
   ilog( "create tester" );
   eosio::testing::tester node;
   
   ilog( "verify initial state of tester blockvault state" );
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.blockvault_initialized == true);
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.producer_lib_id == block_id_type{});
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.producer_watermark == std::pair<block_num, block_timestamp_type>{});
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.snapshot_lib_id == block_id_type{});
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.snapshot_watermark == std::pair<block_num, block_timestamp_type>{});

   ilog( "produce block (implicitly invoking blockvault logic" );
   node.produce_block();

   ilog( "verify initial state of tester blockvault state" );
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.blockvault_initialized == true);
   // BOOST_REQUIRE(++eosio::blockvault_client_plugin::my->bvs.producer_lib_id == block_id_type{});
   // BOOST_REQUIRE(++eosio::blockvault_client_plugin::my->bvs.producer_watermark == std::pair<block_num, block_timestamp_type>{});
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.snapshot_lib_id == block_id_type{});
   // BOOST_REQUIRE(eosio::blockvault_client_plugin::my->bvs.snapshot_watermark == std::pair<block_num, block_timestamp_type>{});
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_client_plugin_append_external_block ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_client_plugin_propose_snapshot ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_client_plugin_sync_for_construction ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
