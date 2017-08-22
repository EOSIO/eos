#include <appbase/application.hpp>

#include <eos/http_plugin/http_plugin.hpp>
#include "eos/wallet_plugin/wallet_plugin.hpp"
#include "eos/wallet_api_plugin/wallet_api_plugin.hpp"

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#include <boost/exception/diagnostic_information.hpp>

using namespace appbase;
using namespace eos;

int main(int argc, char** argv)
{
   try {
      app().register_plugin<http_plugin>();
      app().register_plugin<wallet_plugin>();
      app().register_plugin<wallet_api_plugin>();
      if(!app().initialize<wallet_plugin, wallet_api_plugin, http_plugin>(argc, argv))
         return -1;
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
