/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <appbase/application.hpp>

#include <eos/producer_plugin/producer_plugin.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/http_plugin/http_plugin.hpp>
#include <eos/chain_api_plugin/chain_api_plugin.hpp>
#include <eos/db_plugin/db_plugin.hpp>
#include <eos/net_plugin/net_plugin.hpp>
#include <eos/account_history_plugin/account_history_plugin.hpp>
#include <eos/account_history_api_plugin/account_history_api_plugin.hpp>
#include <eos/wallet_api_plugin/wallet_api_plugin.hpp>
#include <eos/net_api_plugin/net_api_plugin.hpp>
#include <eosio/txn_test_gen_plugin/txn_test_gen_plugin.hpp>
#include <eosio/faucet_testnet_plugin/faucet_testnet_plugin.hpp>

#include <fc/log/logger_config.hpp>
#include <fc/log/appender.hpp>
#include <fc/exception/exception.hpp>

#include <boost/exception/diagnostic_information.hpp>

#include "config.hpp"

using namespace appbase;
using namespace eosio;

namespace fc {
   std::unordered_map<std::string,appender::ptr>& get_appender_map();
}

void initialize_logging()
{
  auto config_path = app().get_logging_conf();
  if(fc::exists(config_path))
    fc::configure_logging(config_path);
  for(auto iter : fc::get_appender_map())
    iter.second->initialize(app().get_io_service());
}

int main(int argc, char** argv)
{
   try {
      app().set_version(eosio::eosd::config::version);
      app().register_plugin<net_api_plugin>();
      app().register_plugin<chain_api_plugin>();
      app().register_plugin<wallet_api_plugin>();
      app().register_plugin<producer_plugin>();
      app().register_plugin<account_history_api_plugin>();
      app().register_plugin<db_plugin>();
      app().register_plugin<txn_test_gen_plugin>();
      app().register_plugin<faucet_testnet_plugin>();
      if(!app().initialize<chain_plugin, http_plugin, net_plugin>(argc, argv))
         return -1;
      initialize_logging();
      ilog("eosd version ${ver}", ("ver", eosio::eosd::config::itoh(static_cast<uint32_t>(app().version()))));
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
