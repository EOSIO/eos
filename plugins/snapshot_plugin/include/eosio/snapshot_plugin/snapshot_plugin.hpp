/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <boost/signals2/connection.hpp>

namespace eosio {

using namespace appbase;

class snapshot_plugin : public appbase::plugin<snapshot_plugin> {
public:
   snapshot_plugin();
   virtual ~snapshot_plugin();
 
   APPBASE_PLUGIN_REQUIRES((chain_plugin))
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class snapshot_plugin_impl> my;
   fc::optional<boost::signals2::scoped_connection> m_irreversible_block_connection;
   fc::optional<boost::signals2::scoped_connection> m_accepted_block_connection;
};

}
