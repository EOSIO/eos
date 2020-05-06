#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain_api_plugin/chain_api_plugin.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;

template<typename T>
auto call_parse_params_no_params_required(const string& body)
{
   return parse_params<T, http_params_types::no_params_required>(body);
}

template<typename T>
auto call_parse_params_params_required(const string& body)
{
   return parse_params<T, http_params_types::params_required>(body);
}

BOOST_AUTO_TEST_SUITE(plugin_tests)
BOOST_AUTO_TEST_CASE( parse_params ) try {
   // empty with no_params_required
   {
      const std::string empty_str;
      BOOST_REQUIRE(empty_str.empty());
      BOOST_REQUIRE_NO_THROW(
         auto test_result = call_parse_params_no_params_required<int>(empty_str);
         BOOST_REQUIRE(test_result == 0);
      );
      BOOST_REQUIRE_THROW(
            call_parse_params_params_required<int>(empty_str), chain::invalid_http_request
      );
   }
   // invalid input
   {
      const std::string invalid_int_str = "#$%";
      BOOST_REQUIRE(!invalid_int_str.empty());
      BOOST_REQUIRE_THROW(
         call_parse_params_no_params_required<int>(invalid_int_str), chain::invalid_http_request
      );
      BOOST_REQUIRE_THROW(
         call_parse_params_params_required<int>(invalid_int_str), chain::invalid_http_request
      );
   }
   // valid input
   {
      const int exp_result = 1234;
      const std::string valid_int_str = std::to_string(exp_result);
      BOOST_REQUIRE(!valid_int_str.empty());
      BOOST_REQUIRE_NO_THROW(
         const auto ret = call_parse_params_no_params_required<int>(valid_int_str);
         BOOST_REQUIRE(ret == exp_result);
      );
      BOOST_REQUIRE_NO_THROW(
         const auto ret = call_parse_params_params_required<int>(valid_int_str);
         BOOST_REQUIRE(ret == exp_result);
      );
   }
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( chain_api_plugin ) try {
   eosio::chain_api_plugin  chain_api;
   chain_api.plugin_startup();
   auto& http = app().get_plugin<http_plugin>();
//   auto& chainPlugin = app().get_plugin<chain_plugin>();
   auto apis_result = http.get_supported_apis();
   std::cout << "Api size: " << apis_result.apis.size() << std::endl;
   for(auto& api : apis_result.apis) {
      std::cout << " [" << api << "] \n";
   }
   std::cout << std::endl;
   eosio::url_response_callback cb_200 = [](const int ret_code, const fc::variant& result) {
      std::cout << "-------hello-----" << std::endl;
      std::cout << "return code: " << ret_code
                << ", result: " << fc::json::to_pretty_string<fc::variant>(result) << std::endl;
      BOOST_REQUIRE(ret_code == 200);
   };
   eosio::url_response_callback cb_202 = [](const int ret_code, const fc::variant& result) {
      std::cout << "return code: " << ret_code
                << ", result: " << fc::json::to_pretty_string<fc::variant>(result) << std::endl;
      BOOST_REQUIRE(ret_code == 202);
   };
   eosio::url_response_callback cb_400 = [](const int ret_code, const fc::variant& result) {
      std::cout << "return code: " << ret_code
                << ", result: " << fc::json::to_pretty_string<fc::variant>(result) << std::endl;
      BOOST_REQUIRE(ret_code == 400);
   };
   eosio::url_response_callback cb_500 = [](const int ret_code, const fc::variant& result) {
      std::cout << "return code: " << ret_code
                << ", result: " << fc::json::to_pretty_string<fc::variant>(result) << std::endl;
      BOOST_REQUIRE(ret_code == 500);
   };
   const string empty_str;
   // get_info
   {
      const string url = "/v1/chain/get_info";
      BOOST_REQUIRE(empty_str.empty());
      BOOST_REQUIRE_NO_THROW(
         http.test_url_handler(url, empty_str, cb_200);
      );
   }
   chain_api.plugin_shutdown();
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
