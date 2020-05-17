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

   The directories to be monitored are configured;
   they can be any combination of data, data/blocks, data/state and
   any directories like those for trace and state history.
   Space threshold can also be configured.

   Alternative solution to blocks directory space exhaustion
   was considered: starting from block_log::append, 
   make sure all exceptions was handled properly. But it was deemed to be
   too tedious and error prone to review all possible excpetions, and
   might likely break concensus. The plugin approach is more generic,
   monitoring any file systems and without modifying existing code.

**/

#include <eosio/resource_monitor_plugin/resource_monitor_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/exception/exception.hpp>
#include <fc/log/logger_config.hpp> // set_os_thread_name

#include <boost/filesystem.hpp> // C++17's filesystem not in our toolchain, use Boost's

#include <thread>

#include <sys/stat.h>

namespace bfs = boost::filesystem;

namespace eosio {
   static appbase::abstract_plugin& _resource_monitor_plugin = app().register_plugin<resource_monitor_plugin>();

class resource_monitor_plugin_impl {
public:

   void set_program_options(options_description&, options_description& cfg) {
      cfg.add_options()
         ( "resource-monitor-directory",  bpo::value<vector<bfs::path>>()->composing(),
           "The directory whose file system to be monitored. This is a mandatory option. It can appear multiple times for different directories")
         ( "resource-monitor-interval-seconds", bpo::value<uint32_t>()->default_value(def_sleep_time_in_secs),
           "Time in seconds between two consecutive checks of resource usage. Should be greater than 1 and less than 60 (one minute)" )
         ( "resource-monitor-space-threshold", bpo::value<uint32_t>()->default_value(def_used_space_threshold_in_percentage),
             "Threshold in terms of percentage of used space vs total space. If used space is over the threshold, a graceful shutdown is initiated. The value should be greater than 1 to less than 99" )
         ;
   }

   void plugin_initialize(const appbase::variables_map& options) {
      ilog("plugin_initialize");

      // We must have at least one directory to monitor 
      EOS_ASSERT(options.count("resource-monitor-directory") > 0, chain::plugin_config_exception, "At least one resource-monitor-directory must be configured");
      const std::vector<bfs::path> dirs = options["resource-monitor-directory"].as<std::vector<bfs::path>>();

      auto interval = options.at("resource-monitor-interval-seconds").as<uint32_t>();
      EOS_ASSERT(interval > 0 && interval < 60, chain::plugin_config_exception,
         "\"resource-monitor-interval-seconds\" must be greater than 0 and less than 60 (1 minute)");
      sleep_time_in_ms = interval * 1000;  // Convert to milliseconds
      ilog("interval = ${interval}", ("interval", interval));

      used_space_threshold_in_percentage = options.at("resource-monitor-space-threshold").as<uint32_t>();
      EOS_ASSERT(used_space_threshold_in_percentage > 0 && used_space_threshold_in_percentage < 100, chain::plugin_config_exception,
         "\"resource-monitor-space-threshold\" must be greater than 0 and less than 100");
      ilog("threshold = ${threshold}", ("threshold", used_space_threshold_in_percentage));

      // Add filesystems containing the directories to the monitored list
      // and set up actual threshold
      for ( auto& dir: dirs ) {
         add_file_system( dir );
      }
   }

   // Start main thread
   void plugin_startup() {
      ilog("Creating and starting monitor thread");
      monitor_thread = std::thread( [this] {
         fc::set_os_thread_name( "resmon" ); // console_appender uses 9 chars for thread name reporting. 
         monitor_task();
      } );
   }

   // System is shutting down.
   void plugin_shutdown() {
      ilog("shutdown...");

      // Signal the monior thread to shut down
      done = true;

      // Wait for the thread to end
      monitor_thread.join();

      ilog("exit shutdown");
   }

private:
   std::thread               monitor_thread;
   std::atomic_bool          done {false};

   static constexpr uint32_t def_sleep_time_in_secs = 2;
   static constexpr uint32_t def_used_space_threshold_in_percentage = 90;

   uint32_t                  sleep_time_in_ms {def_sleep_time_in_secs * 1000}; 
   uint32_t                  used_space_threshold_in_percentage {def_used_space_threshold_in_percentage};

   struct filesystem_info {
      dev_t           st_dev; // device id of file system containing "file_path"
      uintmax_t       min_available_bytes; // minimum number of available bytes the file system must maintain
      bfs::path       path_name;
   };

   // Stores file systems to be monitored. Duplicate
   // file systems are not stored.
   std::vector<filesystem_info> filesystems;

   void monitor_task() {
      ilog("Entering monitor_task");

      while ( true ) {
         if ( is_any_filesystem_space_below_threshold() ) {
            appbase::app().quit(); // This will gracefully stop Nodeos
            break;
         }
         
         std::this_thread::sleep_for( std::chrono::milliseconds( sleep_time_in_ms ));

         if ( done ) {
            break;
         }
      }
   }

   bool is_any_filesystem_space_below_threshold() {
      // Go over each monitored file system
      for (auto& fs: filesystems) {
         boost::system::error_code ec;
         auto info = bfs::space(fs.path_name, ec);
         if ( ec ) {
            // As the system is running and this plugin is not a critical
            // part of the system, we should not exit.
            // Just report the failure and continue;
            wlog( "Unable to get space info for ${path_name}: [code: ${ec}] ${message}",
               ("path_name", fs.path_name.string())
               ("ec", ec.value())
               ("message", ec.message()));

            continue;
         }

         if (info.available < fs.min_available_bytes) {
            wlog("Space usage on ${path}'s file system exceeded configured threshold ${threshold}%, initiate shuting down!!! available: ${available}, Capacity: ${capacity}, min_available_bytes: ${min_available_bytes}", ("path", fs.path_name.string()) ("threshold", used_space_threshold_in_percentage) ("available", info.available) ("capacity", info.capacity) ("min_available_bytes", fs.min_available_bytes));

            return true;
         }
      }

      return false;
   }

   void add_file_system(const bfs::path& path_name) {
      // Get detailed information of the path
      struct stat statbuf;
      auto status = stat(path_name.string().c_str(), &statbuf);
      EOS_ASSERT(status == 0, chain::plugin_config_exception,
                 "Failed to run stat on ${path} with status ${status}", ("path", path_name.string())("status", status));

      // If the file system containing the path is already
      // in the filesystem list, do not add it to the list again
      for (auto& fs: filesystems) {
         if (statbuf.st_dev == fs.st_dev) { // Two files belong to the same file system if their device IDs are the same.
            ilog("file ${path_name}'s file system already in list", ("path_name", path_name.string()));

            return;
         }
      }

      // For efficeincy, precalculate threshold values to avoid calculating it
      // everytime we check space usage. Since bfs::space returns
      // available amount, we use minimum available amount as threshold. 
      boost::system::error_code ec;
      auto info = bfs::space(path_name, ec);
      EOS_ASSERT(!ec, chain::plugin_config_exception,
         "Unable to get space info for ${path_name}: [code: ${ec}] ${message}",
         ("path_name", path_name.string())
         ("ec", ec.value())
         ("message", ec.message()));

      auto min_available_bytes = (100 - used_space_threshold_in_percentage) * (info.capacity / 100); // (100 - used_space_threshold_in_percentage) is the percentage of minimum number of available bytes the file system must maintain  

      // Add to the list
      filesystem_info fs = { statbuf.st_dev, min_available_bytes, path_name };
      filesystems.push_back(fs);
      
      ilog("Added path name: ${path_name}, min_available_bytes: ${min_available_bytes}, capacity: ${capacity}", ("path_name", path_name.string()) ("min_available_bytes", min_available_bytes) ("capacity", info.capacity) );
   }
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

}
