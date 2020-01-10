#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>
#include <eosio/chain/controller.hpp>

namespace eosio {

class login_plugin : public plugin<login_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin)(http_plugin))

   login_plugin();
   virtual ~login_plugin();

   virtual void set_program_options(options_description&, options_description&) override;
   void plugin_initialize(const variables_map&);
   void plugin_startup();
   void plugin_shutdown();

   struct start_login_request_params {
      chain::time_point_sec expiration_time;
   };

   struct start_login_request_results {
      chain::public_key_type server_ephemeral_pub_key;
   };

   struct finalize_login_request_params {
      chain::public_key_type server_ephemeral_pub_key;
      chain::public_key_type client_ephemeral_pub_key;
      chain::permission_level permission;
      std::string data;
      std::vector<chain::signature_type> signatures;
   };

   struct finalize_login_request_results {
      chain::sha256 digest{};
      flat_set<chain::public_key_type> recovered_keys{};
      bool permission_satisfied = false;
      std::string error{};
   };

   struct do_not_use_gen_r1_key_params {};

   struct do_not_use_gen_r1_key_results {
      chain::public_key_type pub_key;
      chain::private_key_type priv_key;
   };

   struct do_not_use_sign_params {
      chain::private_key_type priv_key;
      chain::bytes data;
   };

   struct do_not_use_sign_results {
      chain::signature_type sig;
   };

   struct do_not_use_get_secret_params {
      chain::public_key_type pub_key;
      chain::private_key_type priv_key;
   };

   struct do_not_use_get_secret_results {
      chain::sha512 secret;
   };

   start_login_request_results start_login_request(const start_login_request_params&);
   finalize_login_request_results finalize_login_request(const finalize_login_request_params&);

   do_not_use_gen_r1_key_results do_not_use_gen_r1_key(const do_not_use_gen_r1_key_params&);
   do_not_use_sign_results do_not_use_sign(const do_not_use_sign_params&);
   do_not_use_get_secret_results do_not_use_get_secret(const do_not_use_get_secret_params&);

 private:
   unique_ptr<class login_plugin_impl> my;
};

} // namespace eosio

FC_REFLECT(eosio::login_plugin::start_login_request_params, (expiration_time))
FC_REFLECT(eosio::login_plugin::start_login_request_results, (server_ephemeral_pub_key))
FC_REFLECT(eosio::login_plugin::finalize_login_request_params,
           (server_ephemeral_pub_key)(client_ephemeral_pub_key)(permission)(data)(signatures))
FC_REFLECT(eosio::login_plugin::finalize_login_request_results, (digest)(recovered_keys)(permission_satisfied)(error))

FC_REFLECT_EMPTY(eosio::login_plugin::do_not_use_gen_r1_key_params)
FC_REFLECT(eosio::login_plugin::do_not_use_gen_r1_key_results, (pub_key)(priv_key))
FC_REFLECT(eosio::login_plugin::do_not_use_sign_params, (priv_key)(data))
FC_REFLECT(eosio::login_plugin::do_not_use_sign_results, (sig))
FC_REFLECT(eosio::login_plugin::do_not_use_get_secret_params, (pub_key)(priv_key))
FC_REFLECT(eosio::login_plugin::do_not_use_get_secret_results, (secret))
