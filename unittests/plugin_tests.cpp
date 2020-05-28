#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

template<typename T>
auto call_parse_no_params_required(const string& body)
{
   return parse_params<T, http_params_types::no_params_required>(body);
}

template<typename T>
auto call_parse_params_required(const string& body)
{
   return parse_params<T, http_params_types::params_required>(body);
}

template<typename T>
auto call_parse_possible_no_params(const string& body)
{
   return parse_params<T, http_params_types::possible_no_params>(body);
}

BOOST_AUTO_TEST_SUITE(plugin_tests)

BOOST_AUTO_TEST_CASE( parse_params ) try {
   { // empty body, no input
      const std::string empty_str;
      BOOST_REQUIRE(empty_str.empty());
      BOOST_REQUIRE_NO_THROW(
         auto test_result = call_parse_no_params_required<int>(empty_str);
         BOOST_REQUIRE(test_result == 0);
      );
      BOOST_REQUIRE_NO_THROW(
         auto test_result = call_parse_possible_no_params<std::string>(empty_str);
         BOOST_REQUIRE(test_result == "{}");
      );
      BOOST_REQUIRE_NO_THROW(
            auto test_result = call_parse_no_params_required<std::string>(empty_str);
            BOOST_REQUIRE(test_result == "{}");
      );
      BOOST_REQUIRE_THROW(
            call_parse_params_required<int>(empty_str), chain::invalid_http_request
      );
   }
   // invalid input
   {
      const std::string invalid_int_str = "#$%";
      BOOST_REQUIRE(!invalid_int_str.empty());
      BOOST_REQUIRE_THROW(
         call_parse_no_params_required<int>(invalid_int_str), chain::invalid_http_request
      );
      BOOST_REQUIRE_THROW(
         call_parse_possible_no_params<int>(invalid_int_str), chain::invalid_http_request
      );
      BOOST_REQUIRE_THROW(
         call_parse_params_required<int>(invalid_int_str), chain::invalid_http_request
      );
   }
   // valid input
   {
      const int exp_result = 1234;
      const std::string valid_int_str = std::to_string(exp_result);
      BOOST_REQUIRE(!valid_int_str.empty());
      BOOST_REQUIRE_THROW(
         call_parse_no_params_required<int>(valid_int_str), chain::invalid_http_request
      );
      BOOST_REQUIRE_NO_THROW(
         const auto ret = call_parse_possible_no_params<int>(valid_int_str);
         BOOST_REQUIRE(ret == exp_result);
      );
      BOOST_REQUIRE_NO_THROW(
         const auto ret = call_parse_params_required<int>(valid_int_str);
         BOOST_REQUIRE(ret == exp_result);
      );
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
