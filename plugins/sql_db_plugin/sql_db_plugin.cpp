/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/sql_db_plugin/sql_db_plugin.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <queue>

#include <soci/soci.h>
#include <soci/postgresql/soci-postgresql.h>

namespace {
const char* BUFFER_SIZE_OPTION = "sql_db-queue-size";
const char* SQL_DB_URI_OPTION = "sql_db-uri";
}

namespace fc { class variant; }

namespace eosio {

using chain::account_name;
using chain::action_name;
using chain::block_id_type;
using chain::permission_name;
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::block_trace;
using chain::transaction_id_type;

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

    if (!options.count(SQL_DB_URI_OPTION))
    {
        wlog("db URI not specified => eosio::sql_db_plugin disabled.");
        return;
    }

    std::string uri_str = options.at(SQL_DB_URI_OPTION).as<std::string>();
    ilog("connecting to ${u}", ("u", uri_str));
//    mongocxx::uri uri = mongocxx::uri{uri_str};
//    my->db_name = uri.database();
//    if (my->db_name.empty())
//       my->db_name = "EOS";
//    my->mongo_conn = mongocxx::client{uri};


    //   if (options.count("mongodb-uri")) {
    //      ilog("initializing sql_db_plugin");
    //      my->configured = true;

    //      if (options.at("replay-blockchain").as<bool>()) {
    //         ilog("Replay requested: wiping mongo database on startup");
    //         my->wipe_database_on_startup = true;
    //      }
    //      if (options.at("resync-blockchain").as<bool>()) {
    //         ilog("Resync requested: wiping mongo database on startup");
    //         my->wipe_database_on_startup = true;
    //      }

    //      if (options.count("mongodb-queue-size")) {
    //         auto size = options.at("mongodb-queue-size").as<uint>();
    //         my->queue_size = size;
    //      }

    //      std::string uri_str = options.at("mongodb-uri").as<std::string>();
    //      ilog("connecting to ${u}", ("u", uri_str));
    //      mongocxx::uri uri = mongocxx::uri{uri_str};
    //      my->db_name = uri.database();
    //      if (my->db_name.empty())
    //         my->db_name = "EOS";
    //      my->mongo_conn = mongocxx::client{uri};

    //      // add callback to chain_controller config
    //      chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
    //      FC_ASSERT(chain_plug);
    //      chain_plug->chain_config().applied_block_callbacks.emplace_back(
    //            [my = my](const chain::block_trace& bt) { my->applied_block(bt); });
    //      chain_plug->chain_config().applied_irreversible_block_callbacks.emplace_back(
    //            [my = my](const chain::signed_block& b) { my->applied_irreversible_block(b); });

    //      if (my->wipe_database_on_startup) {
    //         my->wipe_database();
    //      }
    //      my->init();
    //   } else {
    //
    //   }
}

void sql_db_plugin::plugin_startup()
{
    ilog("startup");

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

    // my.reset();
}

} // namespace eosio
