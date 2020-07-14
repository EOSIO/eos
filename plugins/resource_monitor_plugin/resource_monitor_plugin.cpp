/**
   It was reported from a customer that when file system which 
   "data/blocks" belongs to is running out of space, the producer  
   continued to produce blocks and update state but the blocks log was
   "corrupted" in that it no longer contained all the irreversible blocks.
   It was also observed that when file system which "data/state"
   belons to is running out of space, nodeos will crash with SIGBUS as
   the state file is unable to acquire new pages.

   The solution is to have a dedicated plugin to monitor resource 
   usages (file system space now, CPU, memory, and networking
   bandwidth in the future).
   The plugin uses a thread to periodically check space usage of file
   systems of directories being monitored. If space used
   is over a predefined threshold, a graceful shutdown is initiated.
**/

#include <eosio/resource_monitor_plugin/resource_monitor_plugin.hpp>
#include <eosio/resource_monitor_plugin/file_space_handler.hpp>
#include <eosio/resource_monitor_plugin/system_file_space_provider.hpp>

#include <eosio/chain/exceptions.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger_config.hpp> // set_os_thread_name

#include <boost/filesystem.hpp>

#include <thread>

#include <sys/stat.h>

using namespace eosio::resource_monitor;

namespace bfs = boost::filesystem;

namespace eosio {
   static appbase::abstract_plugin& _resource_monitor_plugin = app().register_plugin<resource_monitor_plugin>();

class resource_monitor_plugin_impl {
public:
   resource_monitor_plugin_impl()
      :space_handler(system_file_space_provider(), ctx)
      {
      }

   void set_program_options(options_description&, options_description& cfg) {
      cfg.add_options()
         ( "resource-monitor-interval-seconds", bpo::value<uint32_t>()->default_value(def_interval_in_secs),
           "Time in seconds between two consecutive checks of resource usage. Should be between 1 and 300" )
         ( "resource-monitor-space-threshold", bpo::value<uint32_t>()->default_value(def_space_threshold),
           "Threshold in terms of percentage of used space vs total space. If used space is above (threshold - 5%), a warning is generated.  If used space is above the threshold and resource-monitor-not-shutdown-on-threshold-exceeded is enabled, a graceful shutdown is initiated. The value should be between 6 and 99" )
         ( "resource-monitor-not-shutdown-on-threshold-exceeded",
           "Used to indicate nodeos will not shutdown when threshold is exceeded." )
         ;
   }
   
   void plugin_initialize(const appbase::variables_map& options) {
      dlog("plugin_initialize");
   
      auto interval = options.at("resource-monitor-interval-seconds").as<uint32_t>();
      EOS_ASSERT(interval >= interval_low && interval <= interval_high, chain::plugin_config_exception,
         "\"resource-monitor-interval-seconds\" must be between ${interval_low} and ${interval_high}", ("interval_low", interval_low) ("interval_high", interval_high));
      space_handler.set_sleep_time(interval);
      ilog("Monitoring interval set to ${interval}", ("interval", interval));
   
      auto threshold = options.at("resource-monitor-space-threshold").as<uint32_t>();
      EOS_ASSERT(threshold >= space_threshold_low  && threshold <= space_threshold_high, chain::plugin_config_exception,
         "\"resource-monitor-space-threshold\" must be between ${space_threshold_low} and ${space_threshold_high}", ("space_threshold_low", space_threshold_low) ("space_threshold_high", space_threshold_high));
      space_handler.set_threshold(threshold, threshold - space_threshold_warning_diff);
      ilog("Space usage threshold set to ${threshold}", ("threshold", threshold));
   
      if (options.count("resource-monitor-not-shutdown-on-threshold-exceeded")) {
         // If set, not shutdown
         space_handler.set_shutdown_on_exceeded(false);
         ilog("Shutdown flag when threshold exceeded set to false");
      } else {
         // Default will shut down
         space_handler.set_shutdown_on_exceeded(true);
         ilog("Shutdown flag when threshold exceeded set to true");
      }
   }
   
   // Start main thread
   void plugin_startup() {
      ilog("Creating and starting monitor thread");
   
      // By now all plugins are initialized.
      // Find out filesystems containing the directories requested
      // so far.
      for ( auto& dir: directories_registered ) {
         space_handler.add_file_system( dir );
   
         // A directory like "data" contains subdirectories like
         // "block". Those subdirectories can mount on different
         // file systems. Make sure they are taken care of.
         for (bfs::directory_iterator itr(dir); itr != bfs::directory_iterator(); ++itr) {
            if (fc::is_directory(itr->path())) {
               space_handler.add_file_system( itr->path() );
            }
         }
      }
   
      monitor_thread = std::thread( [this] {
         fc::set_os_thread_name( "resmon" ); // console_appender uses 9 chars for thread name reporting. 
         space_handler.space_monitor_loop();
   
         ctx.run();
      } );
   }
   
   // System is shutting down.
   void plugin_shutdown() {
      ilog("shutdown...");
   
      ctx.stop();
   
      // Wait for the thread to end
      monitor_thread.join();
   
      ilog("exit shutdown");
   }

   void monitor_directory(const bfs::path& path) {
      dlog("${path} registered to be monitored", ("path", path.string()));
      directories_registered.push_back(path);
   }

private:
   std::thread               monitor_thread;
   std::vector<bfs::path>    directories_registered;
   
   static constexpr uint32_t def_interval_in_secs = 2;
   static constexpr uint32_t interval_low = 1;
   static constexpr uint32_t interval_high = 300;

   static constexpr uint32_t def_space_threshold = 90; // in percentage
   static constexpr uint32_t space_threshold_low = 6; // in percentage
   static constexpr uint32_t space_threshold_high = 99; // in percentage
   static constexpr uint32_t space_threshold_warning_diff = 5; // Warning issued when space used reached (threshold - space_threshold_warning_diff). space_threshold_warning_diff must be smaller than space_threshold_low

   boost::asio::io_context     ctx;

   using file_space_handler_t = file_space_handler<system_file_space_provider>;
   file_space_handler_t space_handler;
};

resource_monitor_plugin::resource_monitor_plugin():my(std::make_unique<resource_monitor_plugin_impl>()) {}

resource_monitor_plugin::~resource_monitor_plugin() {}

void resource_monitor_plugin::set_program_options(options_description& cli, options_description& cfg) {
   my->set_program_options(cli, cfg);
}

void resource_monitor_plugin::plugin_initialize(const variables_map& options) {
   my->plugin_initialize(options);
}

void resource_monitor_plugin::plugin_startup() {
   my->plugin_startup();
}

void resource_monitor_plugin::plugin_shutdown() {
   my->plugin_shutdown();
}

void resource_monitor_plugin::monitor_directory(const bfs::path& path) {
   my->monitor_directory( path );
}

} // namespace
