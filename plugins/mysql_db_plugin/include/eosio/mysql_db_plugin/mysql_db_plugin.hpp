/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <appbase/application.hpp>
#include <memory>

namespace eosio {

using mysql_db_plugin_impl_ptr = std::shared_ptr<class mysql_db_plugin_impl>;

/**
 * Provides persistence to mySQL DB for:
 * accounts
 * actions
 * block_states
 * blocks
 * transaction_traces
 * transactions
 *
 *   See data dictionary (DB Schema Definition - EOS API) for description of mySQL DB schema.
 *
 *   If cmake -DBUILD_MYSQL_DB_PLUGIN=true  not specified then this plugin not compiled/included.
 */
class mysql_db_plugin : public plugin<mysql_db_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   mysql_db_plugin();
   virtual ~mysql_db_plugin();
 
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   mysql_db_plugin_impl_ptr my;
};

}
