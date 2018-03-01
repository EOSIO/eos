/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>


namespace eosio {

namespace chain {
class apply_context;
}

using namespace appbase;

/**
 *  This is a debug plugin, intended to serve as a starting point for making new plugins
 */
class debug_plugin : public appbase::plugin<debug_plugin> {
public:
   debug_plugin();
   virtual ~debug_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class debug_plugin_impl> my;
};

}
