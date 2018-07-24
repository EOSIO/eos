/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @author cc32d9 <cc32d9@gmail.com>
 */
#pragma once
#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class zmq_plugin : public appbase::plugin<zmq_plugin> {
public:
   zmq_plugin();
   virtual ~zmq_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class zmq_plugin_impl> my;
};

}
