#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <appbase/application.hpp>

namespace eosio {

class rabbitmq_trx_plugin : public appbase::plugin<rabbitmq_trx_plugin> {

 public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   rabbitmq_trx_plugin();
   virtual ~rabbitmq_trx_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();
   void handle_sighup() override;

 private:
   std::shared_ptr<struct rabbitmq_trx_plugin_impl> my;
};

} // namespace eosio
