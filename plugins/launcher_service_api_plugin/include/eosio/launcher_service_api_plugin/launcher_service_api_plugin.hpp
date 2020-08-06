#pragma once
#include <appbase/application.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/launcher_service_plugin/launcher_service_plugin.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/io/json.hpp>

namespace eosio {

using namespace appbase;

class launcher_service_api_plugin : public appbase::plugin<launcher_service_api_plugin> {
public:
   launcher_service_api_plugin();
   virtual ~launcher_service_api_plugin();

   APPBASE_PLUGIN_REQUIRES((launcher_service_plugin)(http_plugin))
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class launcher_service_api_plugin_impl> my;
};

}
