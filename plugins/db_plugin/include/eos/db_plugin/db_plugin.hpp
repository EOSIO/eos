#pragma once

#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/chain/block.hpp>
#include <appbase/application.hpp>
#include <memory>

namespace fc { class variant; }

namespace eos {

using db_plugin_impl_ptr = std::shared_ptr<class db_plugin_impl>;

class db_plugin : public plugin<db_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   db_plugin();
   virtual ~db_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   // This may only be called after plugin_initialize() and before plugin_startup()!
   void wipe_database();
   void applied_irreversible_block(const chain::signed_block& block);

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   db_plugin_impl_ptr my;
};

}

