/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <appbase/application.hpp>

#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/wallet_api_plugin/wallet_api_plugin.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#include <boost/exception/diagnostic_information.hpp>

#include <pwd.h>
#include "config.hpp"

using namespace appbase;
using namespace eosio;

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
      auto& http = app().get_plugin<http_plugin>();
      http.add_handler("/v1/keosd/stop", [](string, string, url_response_callback cb) { cb(200, "{}"); std::raise(SIGTERM); } );
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
