#include <appbase/application.hpp>

#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/wallet_api_plugin/wallet_api_plugin.hpp>
#include <eosio/version/version.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#include <boost/exception/diagnostic_information.hpp>

#include <pwd.h>
#include "config.hpp"

using namespace appbase;
using namespace eosio;

void configure_logging(const bfs::path& config_path) {
   try {
      try {
         fc::configure_logging(config_path);
      } catch (...) {
         elog("Error reloading logging.json");
         throw;
      }
   } catch (const fc::exception& e) { //
      elog("${e}", ("e", e.to_detail_string()));
   } catch (const boost::exception& e) {
      elog("${e}", ("e", boost::diagnostic_information(e)));
   } catch (const std::exception& e) { //
      elog("${e}", ("e", e.what()));
   } catch (...) {
      // empty
   }
}

void logging_conf_handler() {
   auto config_path = app().get_logging_conf();
   if (fc::exists(config_path)) {
      ilog("Received HUP.  Reloading logging configuration from ${p}.", ("p", config_path.string()));
   } else {
      ilog("Received HUP.  No log config found at ${p}, setting to default.", ("p", config_path.string()));
   }
   configure_logging(config_path);
   fc::log_config::initialize_appenders(app().get_io_service());
}

void initialize_logging() {
   auto config_path = app().get_logging_conf();
   if (fc::exists(config_path))
      fc::configure_logging(config_path); // intentionally allowing exceptions to escape
   fc::log_config::initialize_appenders(app().get_io_service());

   app().set_sighup_callback(logging_conf_handler);
}


bfs::path determine_home_directory()
{
   bfs::path home;
   struct passwd* pwd = getpwuid(getuid());
   if(pwd) {
      home = pwd->pw_dir;
   }
   else {
      home = getenv("HOME");
   }
   if(home.empty())
      home = "./";
   return home;
}

int main(int argc, char** argv)
{
   try {
      app().set_version_string(eosio::version::version_client());
      app().set_full_version_string(eosio::version::version_full());
      bfs::path home = determine_home_directory();
      app().set_default_data_dir(home / "eosio-wallet");
      app().set_default_config_dir(home / "eosio-wallet");
      http_plugin::set_defaults({
         .default_unix_socket_path = keosd::config::key_store_executable_name + ".sock",
         .default_http_port = 0
      });
      app().register_plugin<wallet_api_plugin>();
      if(!app().initialize<wallet_plugin, wallet_api_plugin, http_plugin>(argc, argv))
         return -1;
      initialize_logging();
      auto& http = app().get_plugin<http_plugin>();
      http.add_handler("/v1/" + keosd::config::key_store_executable_name + "/stop", [&a=app()](string, string, url_response_callback cb) { cb(200, fc::variant(fc::variant_object())); a.quit(); } );
      app().startup();
      app().exec();
   } catch (const fc::exception& e) {
      elog("${e}", ("e",e.to_detail_string()));
   } catch (const boost::exception& e) {
      elog("${e}", ("e",boost::diagnostic_information(e)));
   } catch (const std::exception& e) {
      elog("${e}", ("e",e.what()));
   } catch (...) {
      elog("unknown exception");
   }
   return 0;
}
