/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @author Alessandro Siniscalchi <asiniscalchi@gmail.com>
 */
#include <eosio/sql_db_plugin/sql_db_plugin.hpp>

#include "database.h"

namespace {
const char* BLOCK_START_OPTION = "sql_db-block-start";
const char* BUFFER_SIZE_OPTION = "sql_db-queue-size";
const char* SQL_DB_URI_OPTION = "sql_db-uri";
const char* HARD_REPLAY_OPTION = "hard-replay-blockchain";
const char* RESYNC_OPTION = "delete-all-blocks";
const char* REPLAY_OPTION = "replay-blockchain";
}

namespace fc { class variant; }

namespace eosio {

static appbase::abstract_plugin& _sql_db_plugin = app().register_plugin<sql_db_plugin>();

void sql_db_plugin::set_program_options(options_description& cli, options_description& cfg)
{
    dlog("set_program_options");

    cfg.add_options()
            (BUFFER_SIZE_OPTION, bpo::value<uint>()->default_value(256),
             "The queue size between nodeos and SQL DB plugin thread.")
            (BLOCK_START_OPTION, bpo::value<uint32_t>()->default_value(0),
             "The block to start sync.")
            (SQL_DB_URI_OPTION, bpo::value<std::string>(),
             "Sql DB URI connection string"
             " If not specified then plugin is disabled. Default database 'EOS' is used if not specified in URI.")
            ;
}

void sql_db_plugin::plugin_initialize(const variables_map& options)
{
    ilog("initialize");
    try {
        std::string uri_str = options.at(SQL_DB_URI_OPTION).as<std::string>();
        if (uri_str.empty())
        {
            wlog("db URI not specified => eosio::sql_db_plugin disabled.");
            return;
        }
        ilog("connecting to ${u}", ("u", uri_str));
        uint32_t block_num_start = options.at(BLOCK_START_OPTION).as<uint32_t>();
        auto db = std::make_unique<database>(uri_str, block_num_start);

        if (options.at(HARD_REPLAY_OPTION).as<bool>() ||
                options.at(REPLAY_OPTION).as<bool>() ||
                options.at(RESYNC_OPTION).as<bool>() ||
                !db->is_started())
        {
            if (block_num_start == 0) {
                ilog("Resync requested: wiping database");
                if( options.at( RESYNC_OPTION ).as<bool>() ||
                        options.at( REPLAY_OPTION ).as<bool>()) {
                    ilog( "Resync requested: wiping database" );
                    db->wipe();
                }
            }
        }

        m_block_consumer = std::make_unique<consumer<chain::block_state_ptr>>(std::move(db));
        m_irreversible_block_consumer = std::make_unique<consumer<chain::block_state_ptr>>(std::move(db));

        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        FC_ASSERT(chain_plug);
        auto& chain = chain_plug->chain();
        // TODO: irreversible to different queue to just find block & update flag
        //m_irreversible_block_connection.emplace(chain.irreversible_block.connect([=](const chain::block_state_ptr& b) {m_irreversible_block_consumer->push(b);}));
        m_block_connection.emplace(chain.accepted_block.connect([=](const chain::block_state_ptr& b) {m_block_consumer->push(b);}));
    } FC_LOG_AND_RETHROW()
}

void sql_db_plugin::plugin_startup()
{
    ilog("startup");
}

void sql_db_plugin::plugin_shutdown()
{
    ilog("shutdown");
    m_block_connection.reset();
    m_irreversible_block_connection.reset();
}

} // namespace eosio
