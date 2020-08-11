#pragma once
#include <appbase/application.hpp>
#include <eosio/signature_provider_plugin/signature_provider_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

class witness_plugin : public appbase::plugin<witness_plugin> {
public:
   witness_plugin();

   using witness_callback_func = std::function<void(const chain::block_state_ptr& bsp, const chain::signature_type& sig)>;

   APPBASE_PLUGIN_REQUIRES((signature_provider_plugin)(chain_plugin))
   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   /*
    * Add a callback for when a witness signature is created. This function may only be called from a plugin's
    * initialize() or startup() (and on the main thread, of course). The callback will be on a non-main thread.
    * For proper shutdown semantics, you almost certainly want the passed callback to hold a shared_ptr to
    * the object that will be used in the callback. This will allow that object's lifetime (like a plugin_impl)
    * to extend as long as needed by the witness_plugin (longer than a dependent plugin's shutdown() call).
    */
   void add_on_witness_sig(witness_callback_func&& func);

private:
   std::unique_ptr<struct witness_plugin_impl> my;
};

}
