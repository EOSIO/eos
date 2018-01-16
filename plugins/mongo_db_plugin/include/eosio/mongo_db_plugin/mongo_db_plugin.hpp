/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <appbase/application.hpp>
#include <memory>

namespace eosio {

using mongo_db_plugin_impl_ptr = std::shared_ptr<class mongo_db_plugin_impl>;

/**
 * Provides persistence to MongoDB for:
 *   Blocks
 *   Transactions
 *   Actions
 *   Accounts
 *
 *   See data dictionary (DB Schema Definition - EOS API) for description of MongoDB schema.
 *
 *   The goal ultimately is for all chainbase data to be mirrored in MongoDB via a delayed node processing
 *   irreversible blocks. Currently, only Blocks, Transactions, Messages, and Account balance it mirrored.
 *   Chainbase is being rewritten to be multi-threaded. Once chainbase is stable, integration directly with
 *   a mirror database approach can be followed removing the need for the direct processing of Blocks employed
 *   with this implementation.
 *
 *   If MongoDB env not available (#ifndef MONGODB) this plugin is a no-op.
 */
class mongo_db_plugin : public plugin<mongo_db_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   mongo_db_plugin();
   virtual ~mongo_db_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   mongo_db_plugin_impl_ptr my;
};

}

