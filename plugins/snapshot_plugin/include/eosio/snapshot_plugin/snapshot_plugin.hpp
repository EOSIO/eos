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

   /**
    *  snapshot_version must be changed anytime there is a change
    *  on any of the following types:
    *  - eosio::types::chain_id_type
    *  - eosio::chain::genesis_state
    *  - eosio::chain::block_header_state
    *  - eosio::chain::account_object
    *  - eosio::chain::permission_object
    *  - eosio::chain::table_id_object
    *  - eosio::chain::key_value_object
    */
   uint16_t snapshot_version = 0x0001;

private:
   std::unique_ptr<class snapshot_plugin_impl> my;
   fc::optional<boost::signals2::scoped_connection> m_irreversible_block_connection;
   fc::optional<boost::signals2::scoped_connection> m_accepted_block_connection;
};

}
