#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <payloadless/payloadless.wast.hpp>
#include <payloadless/payloadless.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;
using namespace eosio::testing;
using namespace fc;

class payloadless_tester : public TESTER {

};

BOOST_AUTO_TEST_SUITE(payloadless_tests)

BOOST_FIXTURE_TEST_CASE( test_doit, payloadless_tester ) {
   
   create_accounts( {N(payloadless)} );
   set_code( N(payloadless), payloadless_wast );
   set_abi( N(payloadless), payloadless_abi );

   auto trace = push_action(N(payloadless), N(doit), N(payloadless), mutable_variant_object());
   auto msg = trace.action_traces.front().console;
   BOOST_CHECK_EQUAL(msg == "Im a payloadless action", true);
}

BOOST_AUTO_TEST_SUITE_END()
