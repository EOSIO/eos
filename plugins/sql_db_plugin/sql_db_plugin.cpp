/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/sql_db_plugin/sql_db_plugin.hpp>

#include "consumer.h"
#include "database.h"

namespace {
const char* BUFFER_SIZE_OPTION = "sql_db-queue-size";
const char* SQL_DB_URI_OPTION = "sql_db-uri";
const char* RESYNC_OPTION = "resync-blockchain";
const char* REPLAY_OPTION = "replay-blockchain";
}

namespace fc { class variant; }

namespace eosio {

static appbase::abstract_plugin& _sql_db_plugin = app().register_plugin<sql_db_plugin>();

sql_db_plugin::sql_db_plugin()
{

}

sql_db_plugin::~sql_db_plugin()
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

    std::string uri_str = options.at(SQL_DB_URI_OPTION).as<std::string>();
    if (uri_str.empty())
    {
        wlog("db URI not specified => eosio::sql_db_plugin disabled.");
        return;
    }
    ilog("connecting to ${u}", ("u", uri_str));
    auto db = std::make_shared<database>(uri_str);
    //m_consumer_irreversible_block = std::make_unique<consumer<chain::signed_block>>();

    if (options.at(RESYNC_OPTION).as<bool>()) {
        ilog("Resync requested: wiping database");
    }
    else if (options.at(REPLAY_OPTION).as<bool>()) {
        ilog("Replay requested: wiping mongo database on startup");
    }

    //consumer_irreversible_block aaaa;
    //consumer<chain::signed_block> c([&](const std::vector<chain::signed_block>&){});

    consumer_signed_block::consume_function consume_function = [](const std::vector<chain::signed_block>& elements){
        for (const auto& block : elements)
            ilog(block.id().str());
    };

    m_consumer_irreversible_block = std::make_unique<consumer_signed_block>(consume_function);

    chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
    FC_ASSERT(chain_plug);
    //    chain_plug->chain_config().applied_block_callbacks.emplace_back(
    //                [=](const chain::block_trace& t) { m_consumer_irreversible_block->push(t); });
    chain_plug->chain_config().applied_irreversible_block_callbacks.emplace_back(
                [=](const chain::signed_block& b) {m_consumer_irreversible_block->push(b);});
}

void sql_db_plugin::plugin_startup()
{
    ilog("startup");
    // m_consumer_irreversible_block->start();

    //   if (my->configured) {
    //      ilog("starting db plugin");

    //      my->consume_thread = boost::thread([this] { my->consume_blocks(); });

    //      // chain_controller is created and has resynced or replayed if needed
    //      my->startup = false;
    //   }
}

void sql_db_plugin::plugin_shutdown()
{
    ilog("shutdown");

    // m_consumer_irreversible_block->stop();
}

} // namespace eosio
