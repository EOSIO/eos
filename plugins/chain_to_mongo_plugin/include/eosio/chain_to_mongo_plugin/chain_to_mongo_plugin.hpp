/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

class chain_to_mongo_plugin : public plugin<chain_to_mongo_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((http_plugin) (chain_plugin))

   chain_to_mongo_plugin() = default;
   chain_to_mongo_plugin(const chain_to_mongo_plugin&) = delete;
   chain_to_mongo_plugin(chain_to_mongo_plugin&&) = delete;
   chain_to_mongo_plugin& operator=(const chain_to_mongo_plugin&) = delete;
   chain_to_mongo_plugin& operator=(chain_to_mongo_plugin&&) = delete;
   virtual ~chain_to_mongo_plugin() override = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override {}
   void plugin_initialize(const variables_map& vm) {}
   void plugin_startup();
   void plugin_shutdown() {}

private:
};

}
