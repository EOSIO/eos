/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/debug_plugin/debug_plugin.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/wasm_interface_private.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <dlfcn.h>

extern "C" {
   void apply( uint64_t receiver, uint64_t code, uint64_t action );
}

namespace eosio {
static appbase::abstract_plugin& _debug_plugin = app().register_plugin<debug_plugin>();


class debug_plugin_impl {
   public:
      static void dummy_action_handler(chain::apply_context& context);
      static void* lib_handle;
      typedef void (*apply_function_type)(uint64_t, uint64_t, uint64_t);
      static apply_function_type fn_handle;
};

void* debug_plugin_impl::lib_handle = 0;
debug_plugin_impl::apply_function_type debug_plugin_impl::fn_handle = 0;

debug_plugin::debug_plugin():my(new debug_plugin_impl()){}
debug_plugin::~debug_plugin(){}

void debug_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("debug-account-name", bpo::value<string>(), "Contract name to be debugged")
         ("debug-action-name", bpo::value< vector<string> >()->composing(), "Action name to be debugged. Set for each action")
         ("debug-library-name", bpo::value<string>(), "Shared library name to load the compiler contract code for debugging")
         ;
}

void debug_plugin::plugin_initialize(const variables_map& options) {
   if(options.count("debug-account-name")) {
      account_name = options.at("debug-account-name").as< string >();
   }
   if (options.count("debug-action-name")) {
      actions = options.at("debug-action-name").as<vector<string> >();
   }
   if(options.count("debug-library-name")) {
      library_name = options.at("debug-library-name").as< string >();
   }
}

void debug_plugin::plugin_startup() {
   chain::chain_controller& chain = app().get_plugin<chain_plugin>().chain();
   // Register each of the contracts actions as native contracts.
   for (auto action: actions) {
      chain._set_apply_handler( account_name, account_name, action, debug_plugin_impl::dummy_action_handler );
   }
   if (library_name != "") {
      debug_plugin_impl::lib_handle = dlopen(library_name.c_str(), RTLD_LAZY);
      if (debug_plugin_impl::lib_handle) {
         dlerror();

         debug_plugin_impl::fn_handle = (debug_plugin_impl::apply_function_type) dlsym(debug_plugin_impl::lib_handle, "apply");
         const char* dlsym_error = dlerror();
         if (dlsym_error) {
            elog("Cannot load symbol apply: ${error}", ("error", dlsym_error));
         }
      }
      else {
         elog("Cannot load library: ${lib_name}  ${error}", ("lib_name", library_name)("error", dlerror()));
      }
   }
}

void debug_plugin::plugin_shutdown() {
}

void debug_plugin_impl::dummy_action_handler(chain::apply_context& context) {
   chain::chain_controller& chain = app().get_plugin<chain_plugin>().chain();
   const auto &a = chain.get_database().get<chain::account_object, chain::by_name>(context.receiver);

   if (fn_handle != 0 && a.code.size() > 0) {
      chain::native_apply_context = &context;
      fn_handle(uint64_t(context.receiver), uint64_t(context.act.account), uint64_t(context.act.name));
      chain::native_apply_context = 0;
   }
   else {
      eosio::name account_name;
      eosio::name action_name;
      account_name.value = context.act.account;
      action_name.value = context.act.name;
      std::string reason = "wasm code for contract was not loaded";
      if (fn_handle == 0) reason = "Dynamic library was not loaded";

      FC_ASSERT( false, "Unable to execute native debug action ${action} for account ${account}: ${reason}",
                 ("account", account_name.to_string())("action", action_name.to_string())("reason", reason));
   }
}
}
