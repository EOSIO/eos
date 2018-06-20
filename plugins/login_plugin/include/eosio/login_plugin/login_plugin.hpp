/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
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

   start_login_request_results start_login_request(const start_login_request_params&);

 private:
   unique_ptr<class login_plugin_impl> my;
};

} // namespace eosio

FC_REFLECT(eosio::login_plugin::start_login_request_params, (expiration_time))
FC_REFLECT(eosio::login_plugin::start_login_request_results, (server_ephemeral_pub_key))
