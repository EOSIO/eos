#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <multi_index_test/multi_index_test.wast.hpp>
#include <multi_index_test/multi_index_test.abi.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(multi_index_tests)

BOOST_FIXTURE_TEST_CASE( multi_index_load, tester ) try {

   produce_blocks(2);
   create_accounts( {N(multitest)} );
   produce_blocks(2);

   set_code( N(multitest), multi_index_test_wast );
   set_abi( N(multitest), multi_index_test_abi );


   produce_blocks(1);

} FC_LOG_AND_RETHROW() 

BOOST_AUTO_TEST_SUITE_END()
