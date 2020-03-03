#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/state_history/types.hpp>

namespace fc {
class variant;
}

namespace eosio {
using chain::bytes;
using std::shared_ptr;

typedef shared_ptr<struct state_history_plugin_impl> state_history_ptr;

class state_history_plugin : public plugin<state_history_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   state_history_plugin();
   virtual ~state_history_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

 private:
   state_history_ptr my;
};

} // namespace eosio
