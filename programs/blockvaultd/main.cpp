#include <appbase/application.hpp>
#include <eosio/version/version.hpp>

#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/log/appender.hpp>
#include <fc/exception/exception.hpp>
#include <fc/filesystem.hpp>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include "config.hpp"

using namespace appbase;
using namespace eosio;

namespace detail{
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

   void init_logging(){
      auto config_path = app().get_logging_conf();
      if(fc::exists(config_path)) {
         fc::configure_logging(config_path); // allowing exceptions to escape
      }
      else {
         auto cfg = fc::logging_config::default_config();
         fc::configure_logging( ::detail::add_deep_mind_logger(cfg) );
      }

      fc::log_config::initialize_appenders( app().get_io_service() );
   }

   void reinit_logging() noexcept {
      try {
         try {
            init_logging();
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
         elog("unknown error");
      }
   }
}

void initialize_logging()
{
   ::detail::init_logging();
   app().set_sighup_callback([](){
      auto config_path = app().get_logging_conf();
      if( fc::exists(config_path) ) {
         ilog( "Received HUP.  Reloading logging configuration from ${p}.", ("p", config_path.string()) );
      } else {
         ilog( "Received HUP.  No log config found at ${p}, setting to default.", ("p", config_path.string()) );
      }
      ::detail::reinit_logging();
   });
}

enum return_codes {
   OTHER_FAIL        = -2,
   INITIALIZE_FAIL   = -1,
   SUCCESS           = 0,
   BAD_ALLOC         = 1
};

int main(int argc, char** argv){

   try {
      app().set_version(eosio::blockvault::config::version);
      app().set_version_string(eosio::version::version_client());
      app().set_full_version_string(eosio::version::version_full());

      auto root = fc::app_path();
      app().set_default_data_dir(root / "eosio" / blockvault::config::blockvaultd_executable_name / "data" );
      app().set_default_config_dir(root / "eosio" / blockvault::config::blockvaultd_executable_name / "config" );

      initialize_logging();
      ilog( "${name} version ${ver} ${fv}",
            ("name", blockvault::config::blockvaultd_executable_name)("ver", app().version_string())
            ("fv", app().version_string() == app().full_version_string() ? "" : app().full_version_string()) );
      ilog("${name} using configuration file ${c}", ("name", blockvault::config::blockvaultd_executable_name)("c", app().full_config_file_path().string()));
      ilog("${name} data directory is ${d}", ("name", blockvault::config::blockvaultd_executable_name)("d", app().data_dir().string()));
      app().startup();
      app().set_thread_priority_max();
      app().exec();
   } catch (const fc::std_exception_wrapper& e) {
      elog("${e}", ("e", e.to_detail_string()));
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

   ilog("${name} successfully exiting", ("name", blockvault::config::blockvaultd_executable_name));
   return SUCCESS;
}