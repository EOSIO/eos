/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/event_engine_plugin/event_engine_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <fc/exception/exception.hpp>

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

    void accepted_block( const chain::block_state_ptr& );
    void irreversible_block(const chain::block_state_ptr&);
    void accepted_transaction(const chain::transaction_metadata_ptr&);
    void applied_transaction(const chain::transaction_trace_ptr&);
};

event_engine_plugin_impl::event_engine_plugin_impl() {
}

event_engine_plugin_impl::~event_engine_plugin_impl() {
}

void event_engine_plugin_impl::accepted_block( const chain::block_state_ptr& ) {
}

void event_engine_plugin_impl::irreversible_block(const chain::block_state_ptr&) {
}

void event_engine_plugin_impl::accepted_transaction(const chain::transaction_metadata_ptr&) {
}

void event_engine_plugin_impl::applied_transaction(const chain::transaction_trace_ptr&) {
}



event_engine_plugin::event_engine_plugin():my(new event_engine_plugin_impl()){}
event_engine_plugin::~event_engine_plugin(){}

void event_engine_plugin::set_program_options(options_description&, options_description& cfg) {
}

void event_engine_plugin::plugin_initialize(const variables_map& options) {
    try {
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
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   my->accepted_transaction_connection.reset();
   my->applied_transaction_connection.reset();
   ilog("event_engine deinitialized");
}

}
