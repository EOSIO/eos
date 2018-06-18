/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @author Alessandro Siniscalchi <asiniscalchi@gmail.com>
 */
#include <eosio/sql_db_plugin/sql_db_plugin.hpp>

#include "database.h"

#include "consumer_core.h"
#include "irreversible_block_storage.h"
#include "block_storage.h"

namespace {
const char* BUFFER_SIZE_OPTION = "sql_db-queue-size";
const char* SQL_DB_URI_OPTION = "sql_db-uri";
const char* RESYNC_OPTION = "delete-all-blocks";
const char* REPLAY_OPTION = "replay-blockchain";
}

namespace fc { class variant; }

namespace eosio {

static appbase::abstract_plugin& _sql_db_plugin = app().register_plugin<sql_db_plugin>();

sql_db_plugin::sql_db_plugin():
    m_block_consumer(std::make_unique<block_storage>())
{

}

void sql_db_plugin::set_program_options(options_description& cli, options_description& cfg)
{
    dlog("set_program_options");

    cfg.add_options()
            (BUFFER_SIZE_OPTION, bpo::value<uint>()->default_value(256),
             "The queue size between nodeos and SQL DB plugin thread.")
            (SQL_DB_URI_OPTION, bpo::value<std::string>(),
             "Sql DB URI connection string"
             " If not specified then plugin is disabled. Default database 'EOS' is used if not specified in URI.")
            ;
}

void sql_db_plugin::plugin_initialize(const variables_map& options)
{
    ilog("initialize");
    try {

        std::string uri_str = options.at( SQL_DB_URI_OPTION ).as<std::string>();
        if( uri_str.empty()) {
            wlog( "db URI not specified => eosio::sql_db_plugin disabled." );
            return;
        }
        ilog( "connecting to ${u}", ("u", uri_str));

        auto db = std::make_shared<database>( uri_str );


        if( options.at( RESYNC_OPTION ).as<bool>() ||
            options.at( REPLAY_OPTION ).as<bool>()) {
            ilog( "Resync requested: wiping database" );
            db->wipe();
        }

        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        FC_ASSERT( chain_plug );
        auto& chain = chain_plug->chain();

        m_irreversible_block_consumer = std::make_unique < consumer <
                                        chain::block_state_ptr >> (std::make_unique<irreversible_block_storage>( db ));

        // chain.accepted_block.connect([=](const chain::block_state_ptr& b) {m_block_consumer.push(b);});
        m_irreversible_block_connection.emplace( chain.irreversible_block.connect(
              [=]( const chain::block_state_ptr& b ) { m_irreversible_block_consumer->push( b ); } ));
    } FC_LOG_AND_RETHROW()
}

void sql_db_plugin::plugin_startup()
{
    ilog("startup");
}

void sql_db_plugin::plugin_shutdown()
{
    ilog("shutdown");
    m_irreversible_block_connection.reset();
}

} // namespace eosio
