/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/topology_plugin/topology_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

class topology_api_plugin : public plugin<topology_api_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((topology_plugin) (http_plugin))

   topology_api_plugin() = default;
   topology_api_plugin(const topology_api_plugin&) = delete;
   topology_api_plugin(topology_api_plugin&&) = delete;
   topology_api_plugin& operator=(const topology_api_plugin&) = delete;
   topology_api_plugin& operator=(topology_api_plugin&&) = delete;
   virtual ~topology_api_plugin() override = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override {}
   void plugin_initialize(const variables_map& vm);
   void plugin_startup();
   void plugin_shutdown() {}

private:
};

}
