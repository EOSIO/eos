#define BOOST_TEST_MODULE blockvault_sync_strategy
#include <eosio/chain/permission_object.hpp>
#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain_plugin/blockvault_sync_strategy.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace eosio::chain_apis;


BOOST_AUTO_TEST_SUITE(blockvault_sync_strategy_tests)

BOOST_FIXTURE_TEST_CASE(newaccount_test, TESTER) { try {

	//BOOST_TEST_REQUIRE(find_account_name(results, tester_account) == true);

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()

