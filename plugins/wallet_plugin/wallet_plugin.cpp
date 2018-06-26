/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/wallet_plugin/wallet_manager.hpp>

#include <boost/filesystem/path.hpp>
#include <chrono>

#include <fc/io/json.hpp>

namespace fc { class variant; }

namespace eosio {

static appbase::abstract_plugin& _wallet_plugin = app().register_plugin<wallet_plugin>();

wallet_plugin::wallet_plugin()
  : wallet_manager_ptr(new wallet_manager()) {
}

wallet_manager& wallet_plugin::get_wallet_manager() {
   return *wallet_manager_ptr;
}

void wallet_plugin::set_program_options(options_description& cli, options_description& cfg) {
   cfg.add_options()
         ("wallet-dir", bpo::value<boost::filesystem::path>()->default_value("."),
          "The path of the wallet files (absolute path or relative to application data dir)")
         ("unlock-timeout", bpo::value<int64_t>()->default_value(900),
          "Timeout for unlocked wallet in seconds (default 900 (15 minutes)). "
          "Wallets will automatically lock after specified number of seconds of inactivity. "
          "Activity is defined as any wallet command e.g. list-wallets.")
         ("eosio-key", bpo::value<std::string>(),
          "eosio key that will be imported automatically when a wallet is created.")
         ;
}

void wallet_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing wallet plugin");

   if (options.count("wallet-dir")) {
      auto dir = options.at("wallet-dir").as<boost::filesystem::path>();
      if (dir.is_relative())
         wallet_manager_ptr->set_dir(app().data_dir() / dir);
      else
         wallet_manager_ptr->set_dir(dir);
   }
   if (options.count("unlock-timeout")) {
      auto timeout = options.at("unlock-timeout").as<int64_t>();
      std::chrono::seconds t(timeout);
      wallet_manager_ptr->set_timeout(t);
   }
   if (options.count("eosio-key")) {
      std::string eosio_wif_key = options.at("eosio-key").as<std::string>();
      wallet_manager_ptr->set_eosio_key(eosio_wif_key);
   }
}
} // namespace eosio
