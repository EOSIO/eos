#define BOOST_TEST_MODULE test_resmom_impl
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/resource_monitor_plugin/resmon_impl.hpp>

using namespace eosio;
using namespace eosio::resource_monitor;
using namespace boost::system;

namespace bfs = boost::filesystem;

// For program options
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

struct resmom_fixture {
   void set_program_options() {
      options_description dummy;
      _my.set_program_options(dummy, _cfg);
   }

   void initialize(const std::vector<std::string>& args){
      // We only have at most 3 arguments. OK to hardcodied in test
      // programs.
      const char* argv[10];
      // argv[0] is program name, no need to fill in
      for (auto i=0; i<args.size(); ++i) {
         argv[i+1] = args[i].c_str();
      }

      bpo::variables_map options;
      bpo::store(bpo::parse_command_line(args.size()+1, argv, _cfg), options);
      bpo::notify(options);

      _my.plugin_initialize(options);
   }

   void set_options(const std::vector<std::string>& arg) {
      set_program_options();
      initialize(arg);
   }

   void plugin_startup(const std::vector<bfs::path>& dirs, int runTimeSecs=3) {
      set_options({"--resource-monitor-interval-seconds=1"});

      for (auto& dir: dirs) {
         _my.monitor_directory(dir);
      }

      _my.plugin_startup();
      std::this_thread::sleep_for( std::chrono::milliseconds(runTimeSecs*1000) );
      _my.plugin_shutdown();
   }

   resource_monitor_plugin_impl _my;
   options_description _cfg;
};

BOOST_AUTO_TEST_SUITE(resmom_impl_tests)
   BOOST_FIXTURE_TEST_CASE(intervalTooBig, resmom_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-interval-seconds=301"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(intervalTooSmall, resmom_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-interval-seconds=0"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(intervalLowBound, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-interval-seconds=1"}));
   }

   BOOST_FIXTURE_TEST_CASE(intervalMiddle, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-interval-seconds=150"}));
   }

   BOOST_FIXTURE_TEST_CASE(intervalHighBound, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-interval-seconds=300"}));
   }

   BOOST_FIXTURE_TEST_CASE(thresholdTooBig, resmom_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-space-threshold=100"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(thresholdTooSmall, resmom_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-space-threshold=5"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(thresholdLowBound, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-space-threshold=6"}));
   }

   BOOST_FIXTURE_TEST_CASE(thresholdMiddle, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-space-threshold=60"}));
   }

   BOOST_FIXTURE_TEST_CASE(thresholdHighBound, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-space-threshold=99"}));
   }

   BOOST_FIXTURE_TEST_CASE(noShutdown, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-not-shutdown-on-threshold-exceeded"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupNormal, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/tmp"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupDuplicateDirs, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/tmp", "/tmp"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupMultDirs, resmom_fixture)
   {
      // Under "/" are multiple file systems
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/", "/tmp"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupNoExistingDirs, resmom_fixture)
   {
      // "hsdfgd983" a random file and not existing
      BOOST_REQUIRE_THROW( plugin_startup({"/tmp", "hsdfgd983"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(startupLongRun, resmom_fixture)
   {
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/tmp"}, 120));
   }
BOOST_AUTO_TEST_SUITE_END()
