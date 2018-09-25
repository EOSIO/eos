/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>

namespace fc {
class variant;
}

namespace eosio {
using chain::bytes;
using std::shared_ptr;

typedef shared_ptr<class state_history_plugin_impl> impl_ptr;

struct table_delta {
   std::string                             name{};
   std::vector<std::pair<uint64_t, bytes>> rows{};
   std::vector<uint64_t>                   removed{};
};

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
   impl_ptr my;
};

} // namespace eosio

FC_REFLECT(eosio::table_delta, (name)(rows)(removed));
