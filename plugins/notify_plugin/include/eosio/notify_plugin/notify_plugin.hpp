/**
 *  @file
 *  @copyright eospace in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;
using notify_plugin_ptr = std::unique_ptr<class notify_plugin_impl>;

/**
 *   notify_plugin: make notifications to apps on chain.
 */
class notify_plugin : public appbase::plugin<notify_plugin> {
public:
   notify_plugin();
   virtual ~notify_plugin();
 
   APPBASE_PLUGIN_REQUIRES((chain_plugin))
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   notify_plugin_ptr my;
};

}
