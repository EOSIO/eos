/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/event_engine_plugin/event_engine_plugin.hpp>
#include <eosio/event_engine_plugin/messages.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>
#include <fc/io/json.hpp>

#include <fstream>

namespace eosio {
   static appbase::abstract_plugin& _event_engine_plugin = app().register_plugin<event_engine_plugin>();

class abi_info final {
public:
    abi_info() = default;
    abi_info(const account_name& code, abi_def abi, const chain::microseconds& max_time)
    : code_(code) {
        serializer_.set_abi(abi, max_time);
        for(auto& evt: abi.events) {
            events_.emplace(evt.name, evt.type);
        }
        for(auto& act: abi.actions) {
            actions_.emplace(act.name, act.type);
        }
    }

    std::string get_event_type(const chain::event_name& n) const {
        auto itr = events_.find(n);
        return (itr != events_.end()) ? itr->second : std::string();
    }

    std::string get_action_type(const chain::event_name& n) const {
        auto itr = actions_.find(n);
        return (itr != actions_.end()) ? itr->second : std::string();
    }

    fc::variant to_object(
        const string& type, const void* data, const size_t size, const chain::microseconds& max_time
    ) const {
        if (nullptr == data || 0 == size) return fc::variant();

        fc::datastream<const char*> ds(static_cast<const char*>(data), size);
        auto value = serializer_.binary_to_variant(type, ds, max_time);

        return value;
    }
private:
    const account_name code_;
    eosio::chain::abi_serializer serializer_;
    fc::flat_map<chain::event_name,chain::type_name> events_;
    fc::flat_map<chain::action_name,chain::type_name> actions_;
};

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

    std::map<chain::name,abi_info> abi_map;
    controller &db;
    fc::microseconds abi_serializer_max_time;

    void set_abi(name account, const abi_def& abi);
    void accepted_block( const chain::block_state_ptr& );
    void irreversible_block(const chain::block_state_ptr&);
    void accepted_transaction(const chain::transaction_metadata_ptr&);
    void applied_transaction(const chain::transaction_trace_ptr&);

    template<typename Msg>
    void send_message(const Msg& msg) {
        if(dumpstream_opened) {
            dumpstream << fc::json::to_string(fc::variant(msg)) << std::endl;
            dumpstream.flush();
        }
    }

    const abi_info *get_account_abi(name account) {
        auto itr = abi_map.find(account);
        if(itr == abi_map.end()) {
            try {
                const auto& a = db.get_account(account);
                if(a.abi.size() > 0) {
                    abi_info info(account, a.get_abi(), abi_serializer_max_time);
                    itr = abi_map.emplace(account, std::move(info)).first;
                }
            } catch(const fc::exception &err) {
                ilog("Can't process ABI for ${account}: ${err}",
                        ("account", account)("err", err.to_string()));
                return nullptr;
            }
        }

        if(itr == abi_map.end()) {
            ilog("Missing ABI-description for ${account}", ("account", account));
            return nullptr;
        }

        return &itr->second;
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
            auto result = abi->to_object(action_type, act.data.data(), act.data.size(), abi_serializer_max_time);
            return result;
        } catch (const fc::exception &err) {
            ilog("Can't unpack arguments for action ${account}:${action}: ${err}",
                    ("account", act.account)("event", act.name)("err", err.to_string()));
            return fc::variant();
        }
    }

    fc::variant unpack_event_data(const chain::event &evt) {
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
            auto result = abi->to_object(event_type, evt.data.data(), evt.data.size(), abi_serializer_max_time);
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

void event_engine_plugin_impl::set_abi(name account, const abi_def& abi) {
    auto itr = abi_map.find(account);
    if(itr != abi_map.end()) {
        try {
            abi_map.erase(itr);
            abi_info info(account, abi, abi_serializer_max_time);
            abi_map.emplace(account, std::move(info));
            ilog("ABI updated for ${account}", ("account", account));
        } catch(const fc::exception &err) {
            ilog("Can't process ABI for ${account}: ${err}",
                    ("account", account)("err", err.to_string()));
        }
    }
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

void event_engine_plugin_impl::applied_transaction(const chain::transaction_trace_ptr& trx_trace) {
    ilog("Applied trx: ${block_num}, ${id}", ("block_num", trx_trace->block_num)("id", trx_trace->id));

    std::function<void(std::vector<ActionData> &msg, const chain::action_trace&)> process_action_trace = 
    [&](std::vector<ActionData> &msg, const chain::action_trace& trace) {
        ActionData actData;
        actData.receiver = trace.receipt.receiver;
        actData.code = trace.act.account;
        actData.action = trace.act.name;
        actData.args = unpack_action_data(trace.act);
        if(actData.args.is_null()) {
            actData.data = trace.act.data;
        }

        std::vector<chain::name> events;
        for(auto &event: trace.events) {
            events.push_back(event.name);
            EventData evData;
            evData.code = trace.act.account;
            evData.event = event.name;
            evData.args = unpack_event_data(event);
            if(evData.args.is_null()) {
                evData.data = event.data;
            }
            actData.events.push_back(evData);
        }
        ilog("  action: ${contract}:${action} ${events}", ("contract", trace.act.account)("action", trace.act.name)("events", events));
        for(auto &inline_trace: trace.inline_traces) {
            process_action_trace(actData.inlines, inline_trace);
        }
        msg.push_back(std::move(actData));
    };

    ApplyTrxMessage msg(BaseMessage::ApplyTrx, trx_trace);
    for(auto &trace: trx_trace->action_traces) {
        process_action_trace(msg.actions, trace);
    }
    send_message(msg);
}



event_engine_plugin::event_engine_plugin(){}
event_engine_plugin::~event_engine_plugin(){}

void event_engine_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()
        ("event-engine-dumpfile", bpo::value<string>()->default_value(""))
        ;
}

void event_engine_plugin::plugin_initialize(const variables_map& options) {
    try {
        chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
        EOS_ASSERT( chain_plug, chain::missing_chain_plugin_exception, "" );
        auto& chain = chain_plug->chain();

        my.reset(new event_engine_plugin_impl(chain, chain_plug->get_abi_serializer_max_time()));

        std::string dump_filename = options.at("event-engine-dumpfile").as<string>();
        if(dump_filename != "") {
            ilog("Openning dumpfile \"${filename}\"", ("filename", dump_filename));
            my->dumpstream.open(dump_filename, std::ofstream::out | std::ofstream::app);
            EOS_ASSERT(!my->dumpstream.fail(), chain::plugin_config_exception, "Can't open event-engine-dumpfile");
            my->dumpstream_opened = true;
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
        my->setabi_connection.emplace(
                chain.setabi.connect( [&]( const std::tuple<chain::name,const chain::abi_def&> arg) {
                    my->set_abi( std::get<0>(arg), std::get<1>(arg) );
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
