/**
 *  @file
 *  @copyright defined in eosio/LICENSE.txt
 */
#include <appbase/application.hpp>

#include <eosio/producer_plugin/producer_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain_api_plugin/chain_api_plugin.hpp>
//#include <eosio/net_plugin/net_plugin.hpp>
//#include <eosio/account_history_plugin/account_history_plugin.hpp>
//#include <eosio/account_history_api_plugin/account_history_api_plugin.hpp>
#include <eosio/wallet_api_plugin/wallet_api_plugin.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/exception/exception.hpp>

#include <boost/exception/diagnostic_information.hpp>

#include "config.hpp"

using namespace appbase;
using namespace eosio;

int main(int argc, char** argv)
{
   try {
      app().set_version(eosio::eosiod::config::version);
      ilog("eosiod version ${ver}", ("ver", eosio::eosiod::config::itoh(static_cast<uint32_t>(app().version_int()))));
      app().register_plugin<chain_api_plugin>();
      app().register_plugin<producer_plugin>();
//      app().register_plugin<account_history_api_plugin>();
      app().register_plugin<wallet_api_plugin>();
      if(!app().initialize<chain_plugin, http_plugin>(argc, argv))
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
