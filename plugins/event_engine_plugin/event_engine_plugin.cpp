/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <eosio/event_engine_plugin/event_engine_plugin.hpp>
#include <eosio/event_engine_plugin/ee_genesis_container.hpp>
#include <eosio/event_engine_plugin/messages.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <fstream>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/account_abi_info.hpp>

using namespace eosio::chain;

namespace eosio {
   FC_DECLARE_EXCEPTION(ee_extract_genesis_exception, 10000000, "event engine genesis extract exception");

   static appbase::abstract_plugin& _event_engine_plugin = app().register_plugin<event_engine_plugin>();

class event_engine_plugin_impl {
public:
    event_engine_plugin_impl(controller &db, fc::microseconds abi_serializer_max_time);
    ~event_engine_plugin_impl();

    fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
    fc::optional<boost::signals2::scoped_connection> irreversible_block_connection;
    fc::optional<boost::signals2::scoped_connection> accepted_transaction_connection;
    fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;
    fc::optional<boost::signals2::scoped_connection> setabi_connection;

    std::fstream dumpstream;
    bool dumpstream_opened;
    uint32_t genesis_msg_id = 0;

    controller &db;
    fc::microseconds abi_serializer_max_time;
    std::set<account_name> receiver_filter;
    std::vector<bfs::path> genesis_files;
    cyberway::chaindb::account_abi_info account_abi;

    void accepted_block( const chain::block_state_ptr& );
    void irreversible_block(const chain::block_state_ptr&);
    void accepted_transaction(const chain::transaction_metadata_ptr&);
    void applied_transaction(const chain::transaction_trace_ptr&);

    void send_genesis_start();
    void send_genesis_end();
    void send_genesis_file(const bfs::path& genesis_file);

    bool is_handled_contract(const account_name n) const;

private:
    template<typename Msg>
    void send_message(const Msg& msg) {
        if(dumpstream_opened) {
            dumpstream << fc::json::to_string(fc::variant(msg)) << std::endl;
            EOS_ASSERT(!dumpstream.bad(), chain::plugin_config_exception, "Writing to event-engine-dumpfile failed");
            dumpstream.flush();
        }
    }

    void send_genesis_message(const chain::name code, const chain::name name, const fc::variant& data) {
        GenesisDataMessage msg(BaseMessage::GenesisData, genesis_msg_id++, code, name, data);
        send_message(msg);
    }

    const cyberway::chaindb::abi_info* get_account_abi(name code) {
        if (account_abi.code() == code) {
            return &account_abi.abi();
        }

        account_abi = db.chaindb().get_account_abi_info(code);
        if (account_abi.has_abi_info()) {
            return &account_abi.abi();
        }

        ilog("Missing ABI-description for ${account}", ("account", code));
        return nullptr;
    }

    fc::variant unpack_action_data(const chain::action &act) {
        const auto *abi = get_account_abi(act.account);
        if(abi == nullptr) {
            return fc::variant();
        }

        auto action_type = abi->get_action_type(act.name);
        if(action_type == std::string()) {
            ilog("Missing ABI-description for action ${account}:${action}",
                    ("account", act.account)("action", act.name));
            return fc::variant();
        }

        try {
            auto result = abi->to_object(action_type, act.data.data(), act.data.size());
            return result;
        } catch (const fc::exception &err) {
            ilog("Can't unpack arguments for action ${account}:${action}: ${err}",
                    ("account", act.account)("action", act.name)("err", err.to_string()));
            return fc::variant();
        }
    }

    fc::variant unpack_event_data(const chain::event &evt) {
        if(evt.account == name()) {
            if(evt.name == name("senddeferred")) {
                generated_transaction gtx;
                fc::raw::unpack(evt.data, gtx);

                fc::datastream<char*> packed_trx_stream(gtx.packed_trx.data(), gtx.packed_trx.size());
                transaction trx;
                fc::raw::unpack(packed_trx_stream, trx);
                fc::variant trx_object = db.to_variant_with_abi(trx, abi_serializer_max_time);

                fc::variant gtx_object;
                fc::to_variant(gtx, gtx_object);

                fc::mutable_variant_object result(std::move(gtx_object));
                result["trx"] = trx_object;
                return result;
            } else if(evt.name == name("canceldefer")) {
                std::pair<account_name, uint128_t> args;
                fc::raw::unpack<std::pair<account_name, uint128_t>>(evt.data, args);
                return fc::mutable_variant_object()("sender",args.first)("sender_id",args.second);
            }
        }

        const auto *abi = get_account_abi(evt.account);
        if(abi == nullptr) {
            return fc::variant();
        }

        auto event_type = abi->get_event_type(evt.name);
        if(event_type == std::string()) {
            ilog("Missing ABI-description for event ${account}:${event}",
                    ("account", evt.account)("event", evt.name));
            return fc::variant();
        }

        try {
            auto result = abi->to_object(event_type, evt.data.data(), evt.data.size());
            return result;
        } catch (const fc::exception &err) {
            ilog("Can't unpack arguments for event ${account}:${event}: ${err}",
                    ("account", evt.account)("event", evt.name)("err", err.to_string()));
            return fc::variant();
        }
    }
};

event_engine_plugin_impl::event_engine_plugin_impl(controller &db, fc::microseconds abi_serializer_max_time) 
: db(db), abi_serializer_max_time(abi_serializer_max_time)
{
}

event_engine_plugin_impl::~event_engine_plugin_impl() {
}

void event_engine_plugin_impl::accepted_block( const chain::block_state_ptr& state) {
    ilog("Accepted block: ${block_num}", ("block_num", state->block_num));

    AcceptedBlockMessage msg(BlockMessage::AcceptBlock, state);
    send_message(msg);
}

void event_engine_plugin_impl::irreversible_block(const chain::block_state_ptr& state) {
    ilog("Irreversible block: ${block_num}", ("block_num", state->block_num));

    BlockMessage msg(BlockMessage::CommitBlock, state);
    send_message(msg);
}

void event_engine_plugin_impl::accepted_transaction(const chain::transaction_metadata_ptr& trx_meta) {
    ilog("Accepted trx: ${id}, ${signed_id}", ("id", trx_meta->id)("signed_id", trx_meta->signed_id));

    AcceptTrxMessage msg(BaseMessage::AcceptTrx, trx_meta);
    send_message(msg);
}

bool event_engine_plugin_impl::is_handled_contract(const account_name n) const {
    return receiver_filter.empty() || receiver_filter.find(n) != receiver_filter.end();
}

void event_engine_plugin_impl::send_genesis_start() {
    send_genesis_message(core_genesis_code, N(datastart), fc::variant_object());
}

void event_engine_plugin_impl::send_genesis_end() {
    send_genesis_message(core_genesis_code, N(dataend), fc::variant_object());
}

void event_engine_plugin_impl::send_genesis_file(const bfs::path& genesis_file) {
    std::cout << "Reading event engine genesis data from " << genesis_file << "..." << std::endl;
    boost::iostreams::mapped_file mfile;
    mfile.open(genesis_file, boost::iostreams::mapped_file::readonly);
    EOS_ASSERT(mfile.is_open(), ee_extract_genesis_exception, "File not opened");

    const ee_genesis_header &h = *(const ee_genesis_header*)mfile.const_data();
    std::cout << "Header magic: " << h.magic << "; ver: " << h.version << std::endl;
    EOS_ASSERT(h.is_valid(), ee_extract_genesis_exception, "Unknown format of the Genesis state file.");

    fc::datastream<const char*> ds(mfile.const_data()+sizeof(h), mfile.size()-sizeof(h));
    // Read ABI
    abi_def abi;
    abi_serializer serializer;

    fc::raw::unpack(ds, abi);
    serializer.set_abi(abi, abi_serializer_max_time);

    while (ds.remaining()) {
        ee_table_header t;
        fc::raw::unpack(ds, t);
        if (ds.remaining() == 0)
            break;

        std::cout << "Reading " << t.count << " record(s) with type: " << t.abi_type << std::endl;
        for (unsigned i = 0; i < t.count; ++i) {
            fc::variant args = serializer.binary_to_variant(t.abi_type, ds, abi_serializer_max_time);
            send_genesis_message(t.code, t.name, args);
        }
    }
    std::cout << "Done reading Event Engine Genesis data from " << genesis_file << std::endl;
}

void event_engine_plugin_impl::applied_transaction(const chain::transaction_trace_ptr& trx_trace) {
    ilog("Applied trx: ${block_num}, ${id}", ("block_num", trx_trace->block_num)("id", trx_trace->id));

    std::function<void(ApplyTrxMessage &msg, const chain::action_trace&)> process_action_trace = 
    [&](ApplyTrxMessage &msg, const chain::action_trace& trace) {
        if (is_handled_contract(trace.receipt.receiver)) {
            ActionData actData;
            actData.receiver = trace.receipt.receiver;
            actData.code = trace.act.account;
            actData.action = trace.act.name;
            actData.auth = trace.act.authorization;
            actData.args = unpack_action_data(trace.act);
            if(actData.args.is_null()) {
                actData.data = trace.act.data;
            }
    
            std::vector<chain::name> events;
            for(auto &event: trace.events) {
                events.push_back(event.name);
                EventData evData;
                evData.code = event.account;
                evData.event = event.name;
                evData.args = unpack_event_data(event);
                if(evData.args.is_null()) {
                    evData.data = event.data;
                }
                actData.events.push_back(evData);
            }
            msg.actions.push_back(std::move(actData));
            ilog("  action: ${contract}:${action} ${events}", ("contract", trace.act.account)("action", trace.act.name)("events", events));
        }

        for(auto &inline_trace: trace.inline_traces) {
            process_action_trace(msg, inline_trace);
        }
    };

    ApplyTrxMessage msg(BaseMessage::ApplyTrx, trx_trace);
    for(auto &trace: trx_trace->action_traces) {
        process_action_trace(msg, trace);
    }
    send_message(msg);
}



event_engine_plugin::event_engine_plugin(){}
event_engine_plugin::~event_engine_plugin(){}

void event_engine_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()
        ("event-engine-dumpfile", bpo::value<string>()->default_value(""))
        ("event-engine-contract", bpo::value<vector<string>>()->composing()->multitoken(),
         "Smart-contracts for which event_engine will handle events (may specify multiple times)")
        ("event-engine-genesis",  bpo::value<vector<string>>()->composing()->multitoken())
        ;
}

#define LOAD_VALUE_SET(options, name, container) \
if( options.count(name) ) { \
   const auto& ops = options[name].as<std::vector<std::string>>(); \
   std::copy(ops.begin(), ops.end(), std::inserter(container, container.end())); \
}

void event_engine_plugin::plugin_initialize(const variables_map& options) {
    try {
        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        EOS_ASSERT( chain_plug, chain::missing_chain_plugin_exception, "" );
        auto& chain = chain_plug->chain();

        my.reset(new event_engine_plugin_impl(chain, chain_plug->get_abi_serializer_max_time()));

        LOAD_VALUE_SET(options, "event-engine-contract", my->receiver_filter);
        LOAD_VALUE_SET(options, "event-engine-genesis", my->genesis_files);

        std::string dump_filename = options.at("event-engine-dumpfile").as<string>();
        if(dump_filename != "") {
            ilog("Openning dumpfile \"${filename}\"", ("filename", dump_filename));
            my->dumpstream.open(dump_filename, std::ofstream::out | std::ofstream::app);
            EOS_ASSERT(!my->dumpstream.fail(), chain::plugin_config_exception, "Can't open event-engine-dumpfile");
            my->dumpstream_opened = true;

            my->send_genesis_start();
            for(const auto& file: my->genesis_files) {
                my->send_genesis_file(file);
            }
            my->send_genesis_end();
        }

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
}

void event_engine_plugin::plugin_shutdown() {
   // OK, that's enough magic
   if(my->dumpstream_opened) {
       my->dumpstream.close();
       my->dumpstream_opened = false;
   }
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   my->accepted_transaction_connection.reset();
   my->applied_transaction_connection.reset();
   ilog("event_engine deinitialized");
}

}
