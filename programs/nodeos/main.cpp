#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/net_plugin/net_plugin.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/version/version.hpp>

#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/log/appender.hpp>
#include <fc/exception/exception.hpp>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "config.hpp"

using namespace appbase;
using namespace eosio;

namespace detail {

fc::logging_config& add_deep_mind_logger(fc::logging_config& config) {
   config.appenders.push_back(
      fc::appender_config( "deep-mind", "dmlog" )
   );

   fc::logger_config dmlc;
   dmlc.name = "deep-mind";
   dmlc.level = fc::log_level::debug;
   dmlc.enabled = true;
   dmlc.appenders.push_back("deep-mind");

   config.loggers.push_back( dmlc );

   return config;
}

void configure_logging(const bfs::path& config_path)
{
   try {
      try {
         if( fc::exists( config_path ) ) {
            fc::configure_logging( config_path );
         } else {
            auto cfg = fc::logging_config::default_config();

            fc::configure_logging( ::detail::add_deep_mind_logger(cfg) );
         }
      } catch (...) {
         elog("Error reloading logging.json");
         throw;
      }
   } catch (const fc::exception& e) {
      elog("${e}", ("e",e.to_detail_string()));
   } catch (const boost::exception& e) {
      elog("${e}", ("e",boost::diagnostic_information(e)));
   } catch (const std::exception& e) {
      elog("${e}", ("e",e.what()));
   } catch (...) {
      // empty
   }
}

} // namespace detail

void logging_conf_handler()
{
   auto config_path = app().get_logging_conf();
   if( fc::exists( config_path ) ) {
      ilog( "Received HUP.  Reloading logging configuration from ${p}.", ("p", config_path.string()) );
   } else {
      ilog( "Received HUP.  No log config found at ${p}, setting to default.", ("p", config_path.string()) );
   }
   ::detail::configure_logging( config_path );
   fc::log_config::initialize_appenders( app().get_io_service() );
}

void initialize_logging()
{
   auto config_path = app().get_logging_conf();
   if(fc::exists(config_path))
     fc::configure_logging(config_path); // intentionally allowing exceptions to escape
   else {
      auto cfg = fc::logging_config::default_config();

      fc::configure_logging( ::detail::add_deep_mind_logger(cfg) );
   }

   fc::log_config::initialize_appenders( app().get_io_service() );

   app().set_sighup_callback(logging_conf_handler);
}

enum return_codes {
   OTHER_FAIL        = -2,
   INITIALIZE_FAIL   = -1,
   SUCCESS           = 0,
   BAD_ALLOC         = 1,
   DATABASE_DIRTY    = 2,
   FIXED_REVERSIBLE  = SUCCESS,
   EXTRACTED_GENESIS = SUCCESS,
   NODE_MANAGEMENT_SUCCESS = 5
};

int main(int argc, char** argv)
{
   try {
      app().set_version(eosio::nodeos::config::version);
      app().set_version_string(eosio::version::version_client());
      app().set_full_version_string(eosio::version::version_full());

      auto root = fc::app_path();
      app().set_default_data_dir(root / "eosio" / nodeos::config::node_executable_name / "data" );
      app().set_default_config_dir(root / "eosio" / nodeos::config::node_executable_name / "config" );
      http_plugin::set_defaults({
         .default_unix_socket_path = "",
         .default_http_port = 8888
      });
      if(!app().initialize<chain_plugin, net_plugin, producer_plugin>(argc, argv)) {
         const auto& opts = app().get_options();
         if( opts.count("help") || opts.count("version") || opts.count("full-version") || opts.count("print-default-config") ) {
            return SUCCESS;
         }
         return INITIALIZE_FAIL;
      }
      initialize_logging();
      ilog( "${name} version ${ver} ${fv}",
            ("name", nodeos::config::node_executable_name)("ver", app().version_string())
            ("fv", app().version_string() == app().full_version_string() ? "" : app().full_version_string()) );
      ilog("${name} using configuration file ${c}", ("name", nodeos::config::node_executable_name)("c", app().full_config_file_path().string()));
      ilog("${name} data directory is ${d}", ("name", nodeos::config::node_executable_name)("d", app().data_dir().string()));
      app().startup();
      app().set_thread_priority_max();
      app().exec();
   } catch( const extract_genesis_state_exception& e ) {
      return EXTRACTED_GENESIS;
   } catch( const fixed_reversible_db_exception& e ) {
      return FIXED_REVERSIBLE;
   } catch( const node_management_success& e ) {
      return NODE_MANAGEMENT_SUCCESS;
   } catch (const std_exception_wrapper& e) {
      try {
         std::rethrow_exception(e.get_inner_exception());
      } catch (const std::system_error&i) {
         if (chainbase::db_error_code::dirty == i.code().value()) {
            elog("Database dirty flag set (likely due to unclean shutdown): replay required" );
            return DATABASE_DIRTY;
         }
      } catch (...) { }

      elog( "${e}", ("e",e.to_detail_string()));
      return OTHER_FAIL;
   } catch( const fc::exception& e ) {
      elog( "${e}", ("e", e.to_detail_string()));
      return OTHER_FAIL;
   } catch( const boost::interprocess::bad_alloc& e ) {
      elog("bad alloc");
      return BAD_ALLOC;
   } catch( const boost::exception& e ) {
      elog("${e}", ("e",boost::diagnostic_information(e)));
      return OTHER_FAIL;
   } catch( const std::exception& e ) {
      elog("${e}", ("e",e.what()));
      return OTHER_FAIL;
   } catch( ... ) {
      elog("unknown exception");
      return OTHER_FAIL;
   }

   ilog("${name} successfully exiting", ("name", nodeos::config::node_executable_name));
   return SUCCESS;
}
