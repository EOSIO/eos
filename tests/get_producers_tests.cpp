#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <contracts.hpp>

#include <fc/io/fstream.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <array>
#include <utility>

#include <eosio/testing/backing_store_tester_macros.hpp>
//#include <eosio/testing/eosio_system_tester.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

BOOST_AUTO_TEST_SUITE(get_producers_tests)

//using backing_store_ts = boost::mpl::list< eosio_system::eosio_system_tester<TESTER>,eosio_system::eosio_system_tester<ROCKSDB_TESTER> >;
using backing_store_ts = boost::mpl::list< TESTER, ROCKSDB_TESTER >;

BOOST_AUTO_TEST_CASE_TEMPLATE( get_producers, TESTER_T, backing_store_ts) { try {
   TESTER_T t;
   // t.produce_block();

   chain_apis::read_only plugin(*(t.control), {}, fc::microseconds::maximum());
   chain_apis::read_only::get_producers_params params = { .json = true, .lower_bound = "", .limit = 21 };

   auto results = plugin.get_producers(params);
   BOOST_REQUIRE_EQUAL(results.more, "");
   BOOST_REQUIRE_EQUAL(results.rows.size(), 1);

} FC_LOG_AND_RETHROW() } /// get_table_next_key_test

BOOST_AUTO_TEST_SUITE_END()
