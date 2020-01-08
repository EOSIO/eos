#pragma once
#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class chain_api_v2_plugin : public appbase::plugin<chain_api_v2_plugin> {
 public:
   chain_api_v2_plugin();
   virtual ~chain_api_v2_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

 private:
   std::unique_ptr<class chain_api_v2_plugin_impl> my;
};

} // namespace eosio
