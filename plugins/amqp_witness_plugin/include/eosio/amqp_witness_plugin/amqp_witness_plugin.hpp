#pragma once
#include <appbase/application.hpp>
#include <eosio/signature_provider_plugin/signature_provider_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

class amqp_witness_plugin : public appbase::plugin<amqp_witness_plugin> {
public:
   amqp_witness_plugin();

   APPBASE_PLUGIN_REQUIRES((signature_provider_plugin)(chain_plugin))
   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<struct amqp_witness_plugin_impl> my;
};

}
