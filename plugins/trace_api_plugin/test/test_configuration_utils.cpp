#define BOOST_TEST_MODULE trace_configuration_utils
#include <boost/test/included/unit_test.hpp>
#include <list>
#include <boost/filesystem.hpp>

#include <eosio/trace_api_plugin/configuration_utils.hpp>
#include <eosio/trace_api_plugin/test_common.hpp>

using namespace eosio;
using namespace eosio::trace_api_plugin::configuration_utils;

namespace bfs = boost::filesystem;

struct temp_file_fixture {
   temp_file_fixture() {}

   ~temp_file_fixture() {
      for (const auto& p: paths) {
         if (bfs::exists(p)) {
            bfs::remove(p);
         }
      }
   }

   std::string create_temp_file( const std::string& contents ) {
      auto path = bfs::temp_directory_path() / bfs::unique_path();
      auto os = bfs::ofstream(path, std::ios_base::out);
      os << contents;
      os.close();
      return paths.emplace_back(std::move(path)).generic_string();
   }

   std::list<bfs::path> paths;
};

BOOST_AUTO_TEST_SUITE(configuration_utils_tests)
   BOOST_AUTO_TEST_CASE(parse_microseconds_test)
   {
      // basic tests
      BOOST_TEST( parse_microseconds("-1") == fc::microseconds::maximum() );
      BOOST_TEST( parse_microseconds("0")  == fc::microseconds(0) );
      BOOST_TEST( parse_microseconds("1")  == fc::microseconds(1'000'000) );
      BOOST_TEST( parse_microseconds("1s") == fc::microseconds(1'000'000) );
      BOOST_TEST( parse_microseconds("1m") == fc::microseconds(60'000'000) );
      BOOST_TEST( parse_microseconds("1h") == fc::microseconds(3'600'000'000) );
      BOOST_TEST( parse_microseconds("1d") == fc::microseconds(86'400'000'000) );
      BOOST_TEST( parse_microseconds("1w") == fc::microseconds(604'800'000'000) );

      // leading zeros OK!
      BOOST_TEST( parse_microseconds("0001s") == fc::microseconds(1'000'000) );

      // negatives other than -1 not OK!
      BOOST_REQUIRE_THROW(parse_microseconds("-2"), chain::plugin_config_exception);

      // decimals not OK!
      BOOST_REQUIRE_THROW(parse_microseconds("1.0s"), chain::plugin_config_exception);

      // bad suffix not OK
      BOOST_REQUIRE_THROW(parse_microseconds("1y"), chain::plugin_config_exception);

      // letters not OK
      BOOST_REQUIRE_THROW(parse_microseconds("xyz"), chain::plugin_config_exception);

      // Overflow not OK
      BOOST_REQUIRE_THROW(parse_microseconds("15250285w"), chain::plugin_config_exception);

   }

   BOOST_AUTO_TEST_CASE(parse_kv_pairs_test)
   {
      using spair = std::pair<std::string, std::string>;

      // basic
      BOOST_TEST( parse_kv_pairs("a=b") == spair("a", "b") );
      BOOST_TEST( parse_kv_pairs("a==b") == spair("a", "=b") );
      BOOST_TEST( parse_kv_pairs("a={}:\"=") == spair("a", "{}:\"=") );
      BOOST_TEST( parse_kv_pairs("{}:\"=a") == spair("{}:\"", "a") );

      // missing key not OK
      BOOST_REQUIRE_THROW(parse_kv_pairs("=b"), chain::plugin_config_exception);

      // missing value not OK
      BOOST_REQUIRE_THROW(parse_kv_pairs("a="), chain::plugin_config_exception);

      // missing = not OK
      BOOST_REQUIRE_THROW(parse_kv_pairs("a"), chain::plugin_config_exception);

      // emptynot OK
      BOOST_REQUIRE_THROW(parse_kv_pairs(""), chain::plugin_config_exception);

   }

   BOOST_FIXTURE_TEST_CASE(abi_def_from_file_or_str_test, temp_file_fixture)
   {
      auto good_json = std::string("{\"version\" : \"test string please ignore\"}");
      auto good_json_filename = create_temp_file(good_json);

      auto good_abi = chain::abi_def();
      good_abi.version = "test string please ignore";

      auto bad_json  = std::string("{{\"version\":oops\"}");
      auto bad_json_filename = create_temp_file(bad_json);
      auto bad_filename = (bfs::temp_directory_path() / bfs::unique_path()).generic_string();
      auto directory_name = bfs::temp_directory_path().generic_string();

      // good cases
      BOOST_TEST( abi_def_from_file_or_str(good_json) == good_abi );
      BOOST_TEST( abi_def_from_file_or_str(good_json_filename) == good_abi );

      // bad cases
      BOOST_REQUIRE_THROW( abi_def_from_file_or_str(bad_json), chain::json_parse_exception );
      BOOST_REQUIRE_THROW( abi_def_from_file_or_str(bad_json_filename), chain::json_parse_exception );
      BOOST_REQUIRE_THROW( abi_def_from_file_or_str(bad_filename), chain::plugin_config_exception );
      BOOST_REQUIRE_THROW( abi_def_from_file_or_str(directory_name), chain::plugin_config_exception );
   }

BOOST_AUTO_TEST_SUITE_END()
