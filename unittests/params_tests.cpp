#include <boost/test/unit_test.hpp>       /* BOOST_AUTO_TEST_SUITE, etc. */

#include <eosio/testing/tester.hpp>       /* tester */
#include <eosio/chain/exceptions.hpp>     /* config_parse_error */

#include <contracts.hpp>                  /* params_test_wasm, params_test_abi */

using namespace eosio;
using namespace eosio::testing;
using mvo = mutable_variant_object;

/**
 * sets params_test contract and activates all protocol features in default constructor
 */
class params_tester : public tester {
public:
   params_tester() : tester(){}
   params_tester(setup_policy policy) : tester(policy){}

   void setup(){
      //set parameters intrinsics are priviledged so we need system account here
      set_code(config::system_account_name, eosio::testing::contracts::params_test_wasm());
      set_abi(config::system_account_name, eosio::testing::contracts::params_test_abi().data());
      produce_block();
   }

   void action(name action_name, mvo mvo){
      push_action( config::system_account_name, action_name, config::system_account_name, mvo );
      produce_block();
   }
};

/**
 * this class is to setup protocol feature `blockchain_parameters` but not `action_return_value`
 */
class params_tester2 : public params_tester{
public:
   params_tester2() : params_tester(setup_policy::preactivate_feature_and_new_bios){
      const auto& pfm = control->get_protocol_feature_manager();
      const auto& d = pfm.get_builtin_digest(builtin_protocol_feature_t::blockchain_parameters);
      BOOST_REQUIRE(d);
      
      preactivate_protocol_features( {*d} );
      produce_block();
   }

   void setup(){
      params_tester::setup();
   }
};

BOOST_AUTO_TEST_SUITE(params_tests)

BOOST_FIXTURE_TEST_CASE(main_test, params_tester){
   //no throw = success
   action("maintest"_n, mvo());
   //doesn't throw as we have all protocol features activated
   action("throwrvia1"_n, mvo());
   action("throwrvia2"_n, mvo());
}

BOOST_FIXTURE_TEST_CASE(throw_test, params_tester){
   BOOST_CHECK_THROW( [&]{action("setthrow1"_n, mvo());}(), chain::config_parse_error);
   BOOST_CHECK_THROW( [&]{action("setthrow2"_n, mvo());}(), fc::out_of_range_exception);
   BOOST_CHECK_THROW( [&]{action("setthrow3"_n, mvo());}(), chain::action_validate_exception);
   BOOST_CHECK_THROW( [&]{action("getthrow1"_n, mvo());}(), chain::config_parse_error);
   BOOST_CHECK_THROW( [&]{action("getthrow2"_n, mvo());}(), fc::out_of_range_exception);
   BOOST_CHECK_THROW( [&]{action("getthrow3"_n, mvo());}(), chain::config_parse_error);
}

BOOST_FIXTURE_TEST_CASE(throw_test2, params_tester2){
   BOOST_CHECK_THROW( [&]{action("throwrvia1"_n, mvo());}(), chain::unsupported_feature);
   BOOST_CHECK_THROW( [&]{action("throwrvia2"_n, mvo());}(), chain::unsupported_feature);
}

BOOST_AUTO_TEST_SUITE_END()
