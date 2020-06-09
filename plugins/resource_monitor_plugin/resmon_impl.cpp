#include <eosio/resource_monitor_plugin/resmon_impl.hpp>

#include <eosio/chain/exceptions.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger_config.hpp> // set_os_thread_name

#include <boost/filesystem.hpp>

#include <thread>
#include <sys/stat.h>

namespace eosio {

resource_monitor_plugin_impl::resource_monitor_plugin_impl()
   :space_handler(system_file_space_provider(), ctx)
{
}

void resource_monitor_plugin_impl::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
      ( "resource-monitor-interval-seconds", bpo::value<uint32_t>()->default_value(def_interval_in_secs),
        "Time in seconds between two consecutive checks of resource usage. Should be between 1 and 300" )
      ( "resource-monitor-space-threshold", bpo::value<uint32_t>()->default_value(def_space_threshold),
        "Threshold in terms of percentage of used space vs total space. If used space is above (threshold - 5%), a warning is generated.  If used space is above the threshold and resource-monitor-not-shutdown-on-threshold-exceeded is enabled, a graceful shutdown is initiated. The value should be between 6 and 99" )
      ( "resource-monitor-not-shutdown-on-threshold-exceeded",
        "Used to indicate nodeos will not shutdown when threshold is exceeded." )
      ;
}

void resource_monitor_plugin_impl::plugin_initialize(const appbase::variables_map& options) {
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
void resource_monitor_plugin_impl::plugin_startup() {
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
void resource_monitor_plugin_impl::plugin_shutdown() {
   ilog("shutdown...");

   ctx.stop();

   // Wait for the thread to end
   monitor_thread.join();

   ilog("exit shutdown");
}

void resource_monitor_plugin_impl::monitor_directory(const bfs::path& path) {
   dlog("${path} registered to be monitored", ("path", path.string()));
   directories_registered.push_back(path);
}

}
