/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/event_engine_plugin/event_engine_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <fstream>

namespace eosio {
   static appbase::abstract_plugin& _event_engine_plugin = app().register_plugin<event_engine_plugin>();

class event_engine_plugin_impl {
public:
    event_engine_plugin_impl();
    ~event_engine_plugin_impl();

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
    fc::optional<boost::signals2::scoped_connection> irreversible_block_connection;
    fc::optional<boost::signals2::scoped_connection> accepted_transaction_connection;
    fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;

    std::fstream dumpstream;

    void accepted_block( const chain::block_state_ptr& );
    void irreversible_block(const chain::block_state_ptr&);
    void accepted_transaction(const chain::transaction_metadata_ptr&);
    void applied_transaction(const chain::transaction_trace_ptr&);
};

event_engine_plugin_impl::event_engine_plugin_impl() {
}

event_engine_plugin_impl::~event_engine_plugin_impl() {
}

void event_engine_plugin_impl::accepted_block( const chain::block_state_ptr& state) {
    ilog("Accepted block: ${block_num}", ("block_num", state->block_num));
}

void event_engine_plugin_impl::irreversible_block(const chain::block_state_ptr& state) {
    ilog("Irreversible block: ${block_num}", ("block_num", state->block_num));
    using mvo = fc::mutable_variant_object;
    auto event = mvo()("event_name", "irreversible_block")("block_num", state->block_num);
    auto msg = fc::json::to_string(static_cast<fc::variant_object>(event));
    dumpstream << msg << std::endl;
    dumpstream.flush();
}

void event_engine_plugin_impl::accepted_transaction(const chain::transaction_metadata_ptr& trx_meta) {
//    ilog("Accepted trx: ${id}, ${signed_id}", ("id", trx_meta->id)("signed_id", trx_meta->signed_id));
}

void event_engine_plugin_impl::applied_transaction(const chain::transaction_trace_ptr& trx_trace) {
    ilog("Applied trx: ${block_num}, ${id}", ("block_num", trx_trace->block_num)("id", trx_trace->id));

    std::function<void(const chain::action_trace&)> print_action_trace = [&](const chain::action_trace& trace) {
        std::vector<chain::name> events;
        for(auto &event: trace.events) {
            events.push_back(event.name);
        }
        ilog("  action: ${contract}:${action} ${events}", ("contract", trace.act.account)("action", trace.act.name)("events", events));
        for(auto &inline_trace: trace.inline_traces) {
            print_action_trace(inline_trace);
        }
    };

    for(auto &trace: trx_trace->action_traces) {
        print_action_trace(trace);
    }
}



event_engine_plugin::event_engine_plugin():my(new event_engine_plugin_impl()){}
event_engine_plugin::~event_engine_plugin(){}

void event_engine_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()
        ("event-engine-dumpfile", bpo::value<string>()->default_value(""))
        ;
}

void event_engine_plugin::plugin_initialize(const variables_map& options) {
    try {
        std::string dump_filename = options.at("event-engine-dumpfile").as<string>();
        ilog("Openning dumpfile \"${filename}\"", ("filename", dump_filename));
        my->dumpstream.open(dump_filename, std::ofstream::out | std::ofstream::app);
        EOS_ASSERT(!my->dumpstream.fail(), chain::plugin_config_exception, "Can't open event-engine-dumpfile");

        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        EOS_ASSERT( chain_plug, chain::missing_chain_plugin_exception, ""  );
        auto& chain = chain_plug->chain();

        my->accepted_block_connection.emplace( 
                chain.accepted_block.connect( [&]( const chain::block_state_ptr& bs ) {
                    my->accepted_block( bs );
                } ));
        my->irreversible_block_connection.emplace(
                chain.irreversible_block.connect( [&]( const chain::block_state_ptr& bs ) {
                    my->irreversible_block( bs );
                } ));
        my->accepted_transaction_connection.emplace(
                chain.accepted_transaction.connect( [&]( const chain::transaction_metadata_ptr& t ) {
                    my->accepted_transaction( t );
                } ));
        my->applied_transaction_connection.emplace(
                chain.applied_transaction.connect( [&]( const chain::transaction_trace_ptr& t ) {
                    my->applied_transaction( t );
                } ));

        ilog("event_engine initialized");
    }
    FC_LOG_AND_RETHROW()
}

void event_engine_plugin::plugin_startup() {
   // Make the magic happen
}

void event_engine_plugin::plugin_shutdown() {
   // OK, that's enough magic
   my->dumpstream.close();
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   my->accepted_transaction_connection.reset();
   my->applied_transaction_connection.reset();
   ilog("event_engine deinitialized");
}

}
