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
#include <eosio/resource_monitor_plugin/resmon_impl.hpp>
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
}
