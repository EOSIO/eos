/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/debug_plugin/debug_plugin.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

extern "C" {
   void apply( uint64_t code, uint64_t action );
}

namespace eosio {
namespace chain {
   extern apply_context* native_context;
}
static appbase::abstract_plugin& _debug_plugin = app().register_plugin<debug_plugin>();


class debug_plugin_impl {
   public:
      static void dummy_action_handler(chain::apply_context& context);
};

debug_plugin::debug_plugin():my(new debug_plugin_impl()){}
debug_plugin::~debug_plugin(){}

void debug_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void debug_plugin::plugin_initialize(const variables_map& options) {
}

void debug_plugin::plugin_startup() {
   chain::chain_controller& chain = app().get_plugin<chain_plugin>().chain();
   // Register each of the contracts actions as native contracts
   chain._set_apply_handler( "currency", "currency", "issue", debug_plugin_impl::dummy_action_handler );
   chain._set_apply_handler( "currency", "currency", "transfer", debug_plugin_impl::dummy_action_handler );
}

void debug_plugin::plugin_shutdown() {
}

void debug_plugin_impl::dummy_action_handler(chain::apply_context& context) {
   eosio::chain::native_context = &context;
   apply(uint64_t(context.act.account), uint64_t(context.act.name));
   eosio::chain::native_context = 0;
}
}
