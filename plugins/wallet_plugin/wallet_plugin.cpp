/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/wallet_plugin/wallet_manager.hpp>
#include <eosio/wallet_plugin/yubihsm_wallet.hpp>
#include <eosio/wallet_plugin/webauthn_wallet.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/filesystem/path.hpp>
#include <chrono>

#include <fc/io/json.hpp>

namespace fc { class variant; }

namespace eosio {

static appbase::abstract_plugin& _wallet_plugin = app().register_plugin<wallet_plugin>();

wallet_plugin::wallet_plugin() {}

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
         ("yubihsm-url", bpo::value<string>()->value_name("URL"),
          "Override default URL of http://localhost:12345 for connecting to yubihsm-connector")
         ("yubihsm-authkey", bpo::value<uint16_t>()->value_name("key_num"),
          "Enables YubiHSM support using given Authkey")
         ("webauthn-key", bpo::value<vector<string>>()->composing(),
          "Key=Value pairs in the form <public-key>=<credential-id>\n"
          "Where:\n"
          "   <public-key>    \tis a string form of a vaild EOSIO WebAuthn public key\n\n"
          "   <credential-id> \tis a hexstring of the credential id for given public-key\n\n")
         ;
}

void wallet_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing wallet plugin");
   try {
      wallet_manager_ptr = std::make_unique<wallet_manager>();

      if (options.count("wallet-dir")) {
         auto dir = options.at("wallet-dir").as<boost::filesystem::path>();
         if (dir.is_relative())
            wallet_manager_ptr->set_dir(app().data_dir() / dir);
         else
            wallet_manager_ptr->set_dir(dir);
      }
      if (options.count("unlock-timeout")) {
         auto timeout = options.at("unlock-timeout").as<int64_t>();
         EOS_ASSERT(timeout > 0, chain::invalid_lock_timeout_exception, "Please specify a positive timeout ${t}", ("t", timeout));
         std::chrono::seconds t(timeout);
         wallet_manager_ptr->set_timeout(t);
      }
      if (options.count("yubihsm-authkey")) {
         uint16_t key = options.at("yubihsm-authkey").as<uint16_t>();
         string connector_endpoint = "http://localhost:12345";
         if(options.count("yubihsm-url"))
            connector_endpoint = options.at("yubihsm-url").as<string>();
         try {
            wallet_manager_ptr->own_and_use_wallet("YubiHSM", make_unique<yubihsm_wallet>(connector_endpoint, key));
         }FC_LOG_AND_RETHROW()
      }
      if (options.count("webauthn-key")) {
         webauthn_wallet::public_key_credential_id_map key_map;
         const std::vector<std::string> key_cred_pairs = options["webauthn-key"].as<std::vector<std::string>>();
         for (const std::string& key_cred_pair : key_cred_pairs) {
            size_t delim = key_cred_pair.find("=");
            EOS_ASSERT(delim != std::string::npos, plugin_config_exception, "Missing \"=\" in the webauthn key pair");

            std::string cred_id_hexstr = key_cred_pair.substr(delim + 1);
            std::vector<uint8_t> cred_id(cred_id_hexstr.size()/2);
            fc::from_hex(cred_id_hexstr, (char*)cred_id.data(), cred_id.size());

            key_map[public_key_type(key_cred_pair.substr(0, delim))] = cred_id;
         }
         wallet_manager_ptr->own_and_use_wallet("WebAuthn", make_unique<webauthn_wallet>(key_map));
      }
   } FC_LOG_AND_RETHROW()
}

} // namespace eosio
