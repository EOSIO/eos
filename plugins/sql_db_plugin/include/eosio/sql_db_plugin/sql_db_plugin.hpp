/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <appbase/application.hpp>
#include <boost/signals2/connection.hpp>
#include <memory>

#include "consumer.h"

namespace eosio {

/**
 * @author Alessandro Siniscalchi <asiniscalchi@gmail.com>
 *
 * Provides persistence to SQL DB for:
 *   Blocks
 *   Transactions
 *   Actions
 *   Accounts
 *
 *   See data dictionary (DB Schema Definition - EOS API) for description of SQL DB schema.
 *
 *   The goal ultimately is for all chainbase data to be mirrored in SQL DB via a delayed node processing
 *   irreversible blocks. Currently, only Blocks, Transactions, Messages, and Account balance it mirrored.
 *   Chainbase is being rewritten to be multi-threaded. Once chainbase is stable, integration directly with
 *   a mirror database approach can be followed removing the need for the direct processing of Blocks employed
 *   with this implementation.
 *
 *   If SQL DB env not available (#ifndef SQL DB) this plugin is a no-op.
 */
class sql_db_plugin final : public plugin<sql_db_plugin> {
public:
    APPBASE_PLUGIN_REQUIRES((chain_plugin))

    sql_db_plugin();

    virtual void set_program_options(options_description& cli, options_description& cfg) override;

    void plugin_initialize(const variables_map& options);
    void plugin_startup();
    void plugin_shutdown();

private:
    std::unique_ptr<consumer<chain::block_state_ptr>> m_irreversible_block_consumer;
    fc::optional<boost::signals2::scoped_connection> m_irreversible_block_connection;
    consumer<chain::block_state_ptr> m_block_consumer;
};

}

