/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/launcher_service_plugin/httpc.hpp>

namespace eosio {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class launcher_service_plugin : public appbase::plugin<launcher_service_plugin> {
public:
   launcher_service_plugin();
   virtual ~launcher_service_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   fc::variant get_info(std::string url);

private:
   std::unique_ptr<class launcher_service_plugin_impl> my;
};

}
