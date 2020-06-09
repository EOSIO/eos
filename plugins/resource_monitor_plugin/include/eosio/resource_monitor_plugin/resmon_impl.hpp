#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio/resource_monitor_plugin/file_space_handler.hpp>
#include <eosio/resource_monitor_plugin/system_file_space_provider.hpp>

namespace bfs = boost::filesystem;
using namespace eosio::resource_monitor;

namespace eosio {

using namespace appbase;

class resource_monitor_plugin_impl {
public:
   resource_monitor_plugin_impl();

   void set_program_options(appbase::options_description&, appbase::options_description& cfg);

   void plugin_initialize(const appbase::variables_map& options);

   // Start main thread
   void plugin_startup();

   // System is shutting down.
   void plugin_shutdown();

   void monitor_directory(const bfs::path& path);

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

}
