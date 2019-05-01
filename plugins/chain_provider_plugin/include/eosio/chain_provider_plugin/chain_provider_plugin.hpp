/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

/**
 * This plugin determines which plugin will own and manage chain controller.
 * Other plugins that need to access chain controller can depend on this.
 */
class chain_provider_plugin : public appbase::plugin<chain_provider_plugin> {
public:
   template<typename Lambda>
   void plugin_requires( Lambda visitor ) {
      auto chain_provider = get_chain_provider();
      if( chain_provider )
         visitor( *chain_provider );
   }

   chain_provider_plugin();
   virtual ~chain_provider_plugin();
 
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup() {}
   void plugin_shutdown() {}

   abstract_plugin* get_chain_provider()const;

private:
   std::unique_ptr<class chain_provider_plugin_impl> my;
};

}
