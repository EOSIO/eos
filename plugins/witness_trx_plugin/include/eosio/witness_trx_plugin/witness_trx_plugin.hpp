#pragma once
#include <appbase/application.hpp>
#include <eosio/witness_plugin/witness_plugin.hpp>

namespace eosio {

using namespace appbase;

class witness_trx_plugin : public appbase::plugin<witness_trx_plugin> {
public:
   witness_trx_plugin();

   APPBASE_PLUGIN_REQUIRES((witness_plugin))
   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::shared_ptr<struct witness_trx_plugin_impl> my;
};

}
