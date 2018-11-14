/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/statetrack_plugin/statetrack_plugin.hpp>
#include <eosio/statetrack_plugin/statetrack_plugin_impl.hpp>

namespace
{
const char *SENDER_BIND = "st-zmq-sender-bind";
const char *SENDER_BIND_DEFAULT = "tcp://127.0.0.1:3000";
} // namespace

namespace eosio
{

using namespace chain;
using namespace chainbase;

static appbase::abstract_plugin &_statetrack_plugin = app().register_plugin<statetrack_plugin>();

// Plugin implementation

statetrack_plugin::statetrack_plugin() : my(new statetrack_plugin_impl()) {}

void statetrack_plugin::set_program_options(options_description &, options_description &cfg)
{
    cfg.add_options()(SENDER_BIND, bpo::value<string>()->default_value(SENDER_BIND_DEFAULT),
                      "ZMQ Sender Socket binding");
    cfg.add_options()("st-filter-on,f", bpo::value<vector<string>>()->composing(),
                      "Track tables which match code:scope:table.");
    cfg.add_options()("st-filter-out,F", bpo::value<vector<string>>()->composing(),
                      "Do not track tables which match code:scope:table.");
}

void statetrack_plugin::plugin_initialize(const variables_map &options)
{
    ilog("initializing statetrack plugin");

    try
    {
        if (options.count("st-filter-on"))
        {
            auto fo = options.at("st-filter-on").as<vector<string>>();
            for (auto &s : fo)
            {
                std::vector<std::string> v;
                boost::split(v, s, boost::is_any_of(":"));
                EOS_ASSERT(v.size() == 3, fc::invalid_arg_exception, "Invalid value ${s} for --filter-on", ("s", s));
                filter_entry fe{v[0], v[1], v[2]};
                my->filter_on.insert(fe);
            }
        }

        if (options.count("st-filter-out"))
        {
            auto fo = options.at("st-filter-out").as<vector<string>>();
            for (auto &s : fo)
            {
                std::vector<std::string> v;
                boost::split(v, s, boost::is_any_of(":"));
                EOS_ASSERT(v.size() == 3, fc::invalid_arg_exception, "Invalid value ${s} for --filter-out", ("s", s));
                filter_entry fe{v[0], v[1], v[2]};
                my->filter_out.insert(fe);
            }
        }

        my->socket_bind_str = options.at(SENDER_BIND).as<string>();
        if (my->socket_bind_str.empty())
        {
            wlog("zmq-sender-bind not specified => eosio::statetrack_plugin disabled.");
            return;
        }

        my->context = new zmq::context_t(1);
        my->sender_socket = new zmq::socket_t(*my->context, ZMQ_PUSH);
        my->chain_plug = app().find_plugin<chain_plugin>();

        ilog("Bind to ZMQ PUSH socket ${u}", ("u", my->socket_bind_str));
        my->sender_socket->bind(my->socket_bind_str);

        ilog("Bind to ZMQ PUSH socket successful");

        auto &chain = my->chain_plug->chain();
        const database &db = chain.db();

        my->abi_serializer_max_time = my->chain_plug->get_abi_serializer_max_time();

        ilog("Binding database events");

        // account_object events
        my->create_index_events<co_index_type>(db);
        // permisson_object events
        my->create_index_events<po_index_type>(db);
        // permisson_link_object events
        my->create_index_events<plo_index_type>(db);
        // key_value_object events
        my->create_index_events<kv_index_type>(db);

        // table_id_object events
        auto &ti_index = db.get_index<ti_index_type>();

        my->connections.emplace_back(
            fc::optional<connection>(ti_index.applied_remove.connect([&](const table_id_object &tio) {
                my->on_applied_table(db, tio, op_type_enum::TABLE_REMOVE);
            })));

        // transaction and block events

        my->connections.emplace_back(
            fc::optional<connection>(chain.applied_transaction.connect([&](const transaction_trace_ptr &ttp) {
                my->on_applied_transaction(ttp);
            })));

        my->connections.emplace_back(
            fc::optional<connection>(chain.accepted_block.connect([&](const block_state_ptr &bsp) {
                my->on_accepted_block(bsp);
            })));

        my->connections.emplace_back(
            fc::optional<connection>(chain.irreversible_block.connect([&](const block_state_ptr &bsp) {
                my->on_irreversible_block(bsp);
            })));
    }
    FC_LOG_AND_RETHROW()
}

void statetrack_plugin::plugin_startup()
{
}

void statetrack_plugin::plugin_shutdown()
{
    ilog("statetrack plugin shutdown");
    if (!my->socket_bind_str.empty())
    {
        my->sender_socket->disconnect(my->socket_bind_str);
        my->sender_socket->close();
        delete my->sender_socket;
        delete my->context;
        my->sender_socket = nullptr;

        for(auto conn : my->connections) {
            conn->disconnect();
            conn.reset();
        }
    }
}

} // namespace eosio
