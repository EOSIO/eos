#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/symbol.hpp>

#include <currency/currency.wast.hpp>
#include <currency/currency.abi.hpp>

#include <exchange/exchange.wast.hpp>
#include <exchange/exchange.abi.hpp>

#include <eosio.multisig/eosio.multisig.wast.hpp>
#include <eosio.multisig/eosio.multisig.abi.hpp>

#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

BOOST_AUTO_TEST_SUITE(multisig_tests)

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::contracts;

class multisig_tester : public eosio::testing::tester {
   public:

   multisig_tester()
   :tester(),abi_ser(fc::json::from_string(eosio_multisig_abi).as<abi_def>())
   {
      create_account( N(eosio.multisig) );
      set_code( N(eosio.multisig), eosio_multisig_wast );
      
      /// TODO set the contract as privileged.
   }

   private:
      abi_serializer abi_ser;
};

BOOST_AUTO_TEST_CASE( bootstrap ) try {
   multisig_tester t;
} FC_LOG_AND_RETHROW() /// test_api_bootstrap



/**
 *  This test will simply assume a proposed transaction for a non-multisig account
 *
 *  0. create users A and B
 *  1. create a token and issue to user A 
 *  2. B proposes that A transfer token to B
 *  3. A accepts proposal
 *  4. B executes proposal
 *  5. check that B has the funds
 */
BOOST_AUTO_TEST_CASE( basic ) try {
   multisig_tester t;
} FC_LOG_AND_RETHROW() /// test_api_bootstrap


/**
 *  This test will simply assume a proposed transaction for a non-multisig account
 *
 *  0. create users A and B
 *  1. create a token and issue to user A 
 *  2. A creates a new "token" permission level and assigns it to currency::transfer action
 *  2. B proposes that A transfer token to B using A@token permission
 *  3. A accepts proposal using A@token permission
 *  4. B executes proposal
 *  5. check that B has the funds
 *  
 *  It is important in this test that A never use his active or owner key.
 */
BOOST_AUTO_TEST_CASE( basic_sub_permission ) try {
   multisig_tester t;
} FC_LOG_AND_RETHROW() /// test_api_bootstrap


BOOST_AUTO_TEST_SUITE_END()
