#define BOOST_TEST_MODULE test_resmom_plugin
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/resource_monitor_plugin/resource_monitor_plugin.hpp>

using namespace eosio;
using namespace boost::system;

namespace bfs = boost::filesystem;

// For program options
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

struct resmon_fixture {
   void set_program_options() {
      options_description dummy;
      _my.set_program_options(dummy, _cfg);
   }

   void initialize(const std::vector<std::string>& args){
      // We only have at most 3 arguments. OK to hardcodied in test
      // programs.
      const char* argv[10];
      EOS_ASSERT(args.size() < 10, chain::plugin_exception, "number of arguments  (${size}) must be less than 10", ("size", args.size()));

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

   resource_monitor_plugin _my;
   options_description _cfg;
};

BOOST_AUTO_TEST_SUITE(resmon_plugin_tests)
   BOOST_FIXTURE_TEST_CASE(intervalTooBig, resmon_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-interval-seconds=301"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(intervalTooSmall, resmon_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-interval-seconds=0"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(intervalLowBound, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-interval-seconds=1"}));
   }

   BOOST_FIXTURE_TEST_CASE(intervalMiddle, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-interval-seconds=150"}));
   }

   BOOST_FIXTURE_TEST_CASE(intervalHighBound, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-interval-seconds=300"}));
   }

   BOOST_FIXTURE_TEST_CASE(thresholdTooBig, resmon_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-space-threshold=100"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(thresholdTooSmall, resmon_fixture)
   {
      BOOST_REQUIRE_THROW(set_options({"--resource-monitor-space-threshold=5"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(thresholdLowBound, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-space-threshold=6"}));
   }

   BOOST_FIXTURE_TEST_CASE(thresholdMiddle, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-space-threshold=60"}));
   }

   BOOST_FIXTURE_TEST_CASE(thresholdHighBound, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-space-threshold=99"}));
   }

   BOOST_FIXTURE_TEST_CASE(noShutdown, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW(set_options({"--resource-monitor-not-shutdown-on-threshold-exceeded"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupNormal, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/tmp"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupDuplicateDirs, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/tmp", "/tmp"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupMultDirs, resmon_fixture)
   {
      // Under "/" are multiple file systems
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/", "/tmp"}));
   }

   BOOST_FIXTURE_TEST_CASE(startupNoExistingDirs, resmon_fixture)
   {
      // "hsdfgd983" a random file and not existing
      BOOST_REQUIRE_THROW( plugin_startup({"/tmp", "hsdfgd983"}), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(startupLongRun, resmon_fixture)
   {
      BOOST_REQUIRE_NO_THROW( plugin_startup({"/tmp"}, 120));
   }
BOOST_AUTO_TEST_SUITE_END()
