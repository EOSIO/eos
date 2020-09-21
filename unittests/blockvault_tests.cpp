// Run from directory: `/eos/build/`
// ./unittests/unit_test --run_test=blockvault_tests

#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/blockvault/blockvault.hpp>

BOOST_AUTO_TEST_SUITE(blockvault_tests)

BOOST_AUTO_TEST_CASE( blockvault ) try {
   BOOST_REQUIRE(true);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
