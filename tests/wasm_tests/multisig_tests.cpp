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

BOOST_AUTO_TEST_SUITE_END()
