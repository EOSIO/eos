// Run from directory: `/eos/build/`
// ./unittests/unit_test --run_test=blockvault_tests

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/blockvault/blockvault.hpp>

BOOST_AUTO_TEST_SUITE(blockvault_tests)

BOOST_AUTO_TEST_CASE( blockvault_init_plugin ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_propose_constructed_block ) try {
   BOOST_REQUIRE(true);
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_append_external_block ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_sync_for_construction ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( blockvault_propose_snapshot ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
