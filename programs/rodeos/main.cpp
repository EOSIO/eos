#include <appbase/application.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <eosio/version/version.hpp>
#include <fc/exception/exception.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include "cloner_plugin.hpp"
#include "config.hpp"
#include "wasm_ql_plugin.hpp"

using namespace appbase;

namespace detail {

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

} // namespace detail

void logging_conf_handler() {
   auto config_path = app().get_logging_conf();
   if (fc::exists(config_path)) {
      ilog("Received HUP.  Reloading logging configuration from ${p}.", ("p", config_path.string()));
   } else {
      ilog("Received HUP.  No log config found at ${p}, setting to default.", ("p", config_path.string()));
   }
   ::detail::configure_logging(config_path);
   fc::log_config::initialize_appenders(app().get_io_service());
}

void initialize_logging() {
   auto config_path = app().get_logging_conf();
   if (fc::exists(config_path))
      fc::configure_logging(config_path); // intentionally allowing exceptions to escape
   fc::log_config::initialize_appenders(app().get_io_service());

   app().set_sighup_callback(logging_conf_handler);
}

enum return_codes {
   other_fail      = -2,
   initialize_fail = -1,
   success         = 0,
   bad_alloc       = 1,
};

int main(int argc, char** argv) {
   try {
      app().set_version(b1::rodeos::config::version);
      app().set_version_string(eosio::version::version_client());
      app().set_full_version_string(eosio::version::version_full());

      auto root = fc::app_path();
      app().set_default_data_dir(root / "eosio" / b1::rodeos::config::rodeos_executable_name / "data");
      app().set_default_config_dir(root / "eosio" / b1::rodeos::config::rodeos_executable_name / "config");
      if (!app().initialize<b1::cloner_plugin, b1::wasm_ql_plugin>(argc, argv)) {
         const auto& opts = app().get_options();
         if (opts.count("help") || opts.count("version") || opts.count("full-version") ||
             opts.count("print-default-config")) {
            return success;
         }
         return initialize_fail;
      }
      initialize_logging();
      ilog("${name} version ${ver} ${fv}",
           ("name", b1::rodeos::config::rodeos_executable_name)("ver", app().version_string())(
                 "fv", app().version_string() == app().full_version_string() ? "" : app().full_version_string()));
      ilog("${name} using configuration file ${c}",
           ("name", b1::rodeos::config::rodeos_executable_name)("c", app().full_config_file_path().string()));
      ilog("${name} data directory is ${d}",
           ("name", b1::rodeos::config::rodeos_executable_name)("d", app().data_dir().string()));
      app().startup();
      app().set_thread_priority_max();
      app().exec();
   } catch (const fc::std_exception_wrapper& e) {
      elog("${e}", ("e", e.to_detail_string()));
      return other_fail;
   } catch (const fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
      return other_fail;
   } catch (const boost::interprocess::bad_alloc& e) {
      elog("bad alloc");
      return bad_alloc;
   } catch (const boost::exception& e) {
      elog("${e}", ("e", boost::diagnostic_information(e)));
      return other_fail;
   } catch (const std::exception& e) {
      elog("${e}", ("e", e.what()));
      return other_fail;
   } catch (...) {
      elog("unknown exception");
      return other_fail;
   }

   ilog("bind-a-lot successfully exiting");
   return success;
}
