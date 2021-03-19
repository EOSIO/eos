#include <eosio/amqp_trace_plugin/amqp_trace_plugin.hpp>
#include <eosio/amqp_trace_plugin/amqp_trace_plugin_impl.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/resource_monitor_plugin/resource_monitor_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/thread_utils.hpp>

#include <boost/signals2/connection.hpp>

namespace {

static appbase::abstract_plugin& amqp_trace_plugin_ = appbase::app().register_plugin<eosio::amqp_trace_plugin>();

} // anonymous

namespace eosio {

amqp_trace_plugin::amqp_trace_plugin()
: my(std::make_shared<amqp_trace_plugin_impl>()) {
   app().register_config_type<amqp_trace_plugin_impl::reliable_mode>();
}

amqp_trace_plugin::~amqp_trace_plugin() {}

void amqp_trace_plugin::publish_error( std::string tid, int64_t error_code, std::string error_message ) {
   my->publish_error( std::string(), std::move(tid), error_code, std::move(error_message) );
}

void amqp_trace_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("amqp-trace-address", bpo::value<std::string>(),
      "AMQP address: Format: amqp://USER:PASSWORD@ADDRESS:PORT\n"
      "Will publish to amqp-trace-queue-name ('trace') queue.");
   op("amqp-trace-queue-name", bpo::value<std::string>()->default_value("trace"),
      "AMQP queue to publish transaction traces.");
   op("amqp-trace-exchange", bpo::value<std::string>()->default_value(""),
      "Existing AMQP exchange to send transaction trace messages.");
   op("amqp-trace-reliable-mode", bpo::value<amqp_trace_plugin_impl::reliable_mode>()->default_value(amqp_trace_plugin_impl::reliable_mode::queue),
      "Reliable mode 'exit', exit application on any AMQP publish error.\n"
      "Reliable mode 'queue', queue transaction traces to send to AMQP on connection establishment.\n"
      "Reliable mode 'log', log an error and drop message when unable to directly publish to AMQP.");
}

void amqp_trace_plugin::plugin_initialize(const variables_map& options) {
   try {
      EOS_ASSERT( options.count("amqp-trace-address"), chain::plugin_config_exception, "amqp-trace-address required" );
      my->amqp_trace_address = options.at("amqp-trace-address").as<std::string>();
      my->amqp_trace_queue_name = options.at("amqp-trace-queue-name").as<std::string>();
      EOS_ASSERT( !my->amqp_trace_queue_name.empty(), chain::plugin_config_exception, "amqp-trace-queue-name required" );
      my->amqp_trace_exchange = options.at("amqp-trace-exchange").as<std::string>();
      my->pub_reliable_mode = options.at("amqp-trace-reliable-mode").as<amqp_trace_plugin_impl::reliable_mode>();
   }
   FC_LOG_AND_RETHROW()
}

void amqp_trace_plugin::plugin_startup() {
      handle_sighup();
   ilog( "Starting amqp_trace_plugin" );

   const boost::filesystem::path trace_data_dir_path = appbase::app().data_dir() / "amqp_trace_plugin";
   const boost::filesystem::path trace_data_file_path = trace_data_dir_path / "trace.bin";
   if( my->pub_reliable_mode != amqp_trace_plugin_impl::reliable_mode::queue ) {
      EOS_ASSERT( !fc::exists( trace_data_file_path ), chain::plugin_config_exception,
                  "Existing queue file when amqp-trace-reliable-mode != 'queue': ${f}",
                  ("f", trace_data_file_path.generic_string()) );
   } else if( auto resmon_plugin = app().find_plugin<resource_monitor_plugin>() ) {
      resmon_plugin->monitor_directory( trace_data_dir_path );
   }

   my->amqp_trace.emplace( my->amqp_trace_address, my->amqp_trace_exchange, my->amqp_trace_queue_name,
                           trace_data_file_path,
                           []( const std::string& err ) {
                              elog( "AMQP fatal error: ${e}", ("e", err) );
                              appbase::app().quit();
                           } );

   auto chain_plug = app().find_plugin<chain_plugin>();
   EOS_ASSERT( chain_plug, chain::missing_chain_plugin_exception, "chain_plugin required" );

   applied_transaction_connection.emplace(
         chain_plug->chain().applied_transaction.connect(
               [me = my]( std::tuple<const chain::transaction_trace_ptr&, const chain::packed_transaction_ptr&> t ) {
                  me->on_applied_transaction( std::get<0>( t ), std::get<1>( t ) );
               } ) );

}

void amqp_trace_plugin::plugin_shutdown() {
   applied_transaction_connection.reset();
   dlog( "exit amqp_trace_plugin" );
}

void amqp_trace_plugin::handle_sighup() {
}

} // namespace eosio
