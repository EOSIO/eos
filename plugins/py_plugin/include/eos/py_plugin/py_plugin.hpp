#pragma once
#include <appbase/application.hpp>

namespace eos {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class py_plugin : public appbase::plugin<py_plugin> {
public:
   py_plugin();
   virtual ~py_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class py_plugin_impl> my;
};

}
