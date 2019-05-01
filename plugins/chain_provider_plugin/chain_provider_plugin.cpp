/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain_provider_plugin/chain_provider_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

static appbase::abstract_plugin& _chain_provider_plugin = app().register_plugin<chain_provider_plugin>();

class chain_provider_plugin_impl {
   public:
      abstract_plugin* chain_provider; // do not access before plugin_initialize()!
};

chain_provider_plugin::chain_provider_plugin():my(new chain_provider_plugin_impl()){}
chain_provider_plugin::~chain_provider_plugin(){}

void chain_provider_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("chain-provider", bpo::value<string>()->default_value("eosio::chain_plugin"),
          "The plugin which owns and manages chain controller")
         ;
}

void chain_provider_plugin::plugin_initialize(const variables_map& options) try {
   if( options.count( "chain-provider" )) {
      const auto& chain_provider_name = options["chain-provider"].as<std::string>();

      if( chain_provider_name != "eosio::chain_plugin" ) {
         ilog("initializing chain controller by ${provider}", ("provider", chain_provider_name));
         my->chain_provider = app().find_plugin(chain_provider_name);
      } else {
         my->chain_provider = &app().register_plugin<chain_plugin>();
      }

      my->chain_provider->initialize(options);
   }
} FC_LOG_AND_RETHROW()

abstract_plugin* chain_provider_plugin::get_chain_provider()const {
   return my->chain_provider;
}

}
