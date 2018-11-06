/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

class statetrack_plugin : public plugin<statetrack_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   statetrack_plugin();
   statetrack_plugin(const statetrack_plugin&) = delete;
   statetrack_plugin(statetrack_plugin&&) = delete;
   statetrack_plugin& operator=(const statetrack_plugin&) = delete;
   statetrack_plugin& operator=(statetrack_plugin&&) = delete;
   virtual ~statetrack_plugin() override = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override;
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class statetrack_plugin_impl> my;
};

}

