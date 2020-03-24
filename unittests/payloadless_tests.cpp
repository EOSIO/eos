#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

class payloadless_tester : public TESTER {

};

BOOST_AUTO_TEST_SUITE(payloadless_tests)

BOOST_FIXTURE_TEST_CASE( test_doit, payloadless_tester ) {
   
   create_accounts( {N(payloadless)} );
   set_code( N(payloadless), contracts::payloadless_wasm() );
   set_abi( N(payloadless), contracts::payloadless_abi().data() );

   auto trace = push_action(N(payloadless), N(doit), N(payloadless), mutable_variant_object());
   auto msg = trace->action_traces.front().console;
   BOOST_CHECK_EQUAL(msg == "Im a payloadless action", true);
}

// test GH#3916 - contract api action with no parameters fails when called from cleos
// abi_serializer was failing when action data was empty.
BOOST_FIXTURE_TEST_CASE( test_abi_serializer, payloadless_tester ) {

   create_accounts( {N(payloadless)} );
   set_code( N(payloadless), contracts::payloadless_wasm() );
   set_abi( N(payloadless), contracts::payloadless_abi().data() );

   variant pretty_trx = fc::mutable_variant_object()
      ("actions", fc::variants({
         fc::mutable_variant_object()
            ("account", name(N(payloadless)))
            ("name", "doit")
            ("authorization", fc::variants({
               fc::mutable_variant_object()
                  ("actor", name(N(payloadless)))
                  ("permission", name(config::active_name))
            }))
            ("data", fc::mutable_variant_object()
            )
         })
     );

   signed_transaction trx;
   // from_variant is key to this test as abi_serializer was explicitly not allowing empty "data"
   abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer::create_yield_function( abi_serializer_max_time ));
   set_transaction_headers(trx);

   trx.sign( get_private_key( N(payloadless), "active" ), control->get_chain_id() );
   auto trace = push_transaction( trx );
   auto msg = trace->action_traces.front().console;
   BOOST_CHECK_EQUAL(msg == "Im a payloadless action", true);
}

BOOST_AUTO_TEST_SUITE_END()
