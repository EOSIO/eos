#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

class resource_monitor_plugin : public appbase::plugin<resource_monitor_plugin> {
public:
   resource_monitor_plugin( );
   virtual ~resource_monitor_plugin();
 
   APPBASE_PLUGIN_REQUIRES( (chain_plugin) )
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class resource_monitor_plugin_impl> my;
};

}
