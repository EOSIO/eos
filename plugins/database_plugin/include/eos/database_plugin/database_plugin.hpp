/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>

#include <chainbase/chainbase.hpp>

namespace eosio {
using namespace appbase;

/**
 * This is a template plugin, intended to serve as a starting point for making new plugins
 */
class database_plugin : public appbase::plugin<database_plugin> {
public:
   database_plugin();
   virtual ~database_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;

   // This may only be called after plugin_initialize() and before plugin_startup()!
   void wipe_database();

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();


   // This may only be called after plugin_startup()!
   chainbase::database& db();
   // This may only be called after plugin_startup()!
   const chainbase::database& db() const;

private:
   std::unique_ptr<class database_plugin_impl> my;
};

}
