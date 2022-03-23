#include <eosio/amqp_trx_plugin/amqp_trx_plugin.hpp>
#include <eosio/amqp_trx_plugin/fifo_trx_processing_queue.hpp>
#include <eosio/amqp/amqp_handler.hpp>
#include <eosio/amqp_trx_plugin/amqp_trace_plugin_impl.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/resource_monitor_plugin/resource_monitor_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/thread_utils.hpp>

#include <fc/log/trace.hpp>

#include <boost/signals2/connection.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace {

static appbase::abstract_plugin& amqp_trx_plugin_ = appbase::app().register_plugin<eosio::amqp_trx_plugin>();

enum class ack_mode {
   received,
   executed,
   in_block
};

std::istream& operator>>(std::istream& in, ack_mode& m) {
   std::string s;
   in >> s;
   if( s == "received" )
      m = ack_mode::received;
   else if( s == "executed" )
      m = ack_mode::executed;
   else if( s == "in_block" )
      m = ack_mode::in_block;
   else
      in.setstate( std::ios_base::failbit );
   return in;
}

std::ostream& operator<<(std::ostream& osm, ack_mode m) {
   if( m == ack_mode::received )
      osm << "received";
   else if( m == ack_mode::executed )
      osm << "executed";
   else if( m == ack_mode::in_block )
      osm << "in_block";
   return osm;
}

} // anonymous

namespace eosio {

using boost::signals2::scoped_connection;

struct amqp_trx_plugin_impl : std::enable_shared_from_this<amqp_trx_plugin_impl> {

   chain_plugin* chain_plug = nullptr;
   producer_plugin* prod_plugin = nullptr;
   std::optional<amqp_handler> amqp_trx;
   amqp_trace_plugin_impl trace_plug;

   std::string amqp_trx_address;
   std::string amqp_trx_queue;
   ack_mode acked = ack_mode::executed;
   std::map<uint32_t, eosio::amqp_handler::delivery_tag_t> tracked_delivery_tags; // block, highest delivery_tag for block
   std::string block_uuid;
   std::set<std::string> tracked_block_uuid_rks;
   uint32_t trx_processing_queue_size = 1000;
   uint32_t trx_retry_interval_us = 500 * 1000; // 500 milliseconds
   uint32_t trx_retry_timeout_us = 60 * 1000 * 1000; // 60 seconds
   bool allow_speculative_execution = false;
   bool started_consuming = false;
   std::shared_ptr<fifo_trx_processing_queue<producer_plugin>> trx_queue_ptr;

   std::optional<scoped_connection> block_start_connection;
   std::optional<scoped_connection> block_abort_connection;
   std::optional<scoped_connection> accepted_block_connection;


   // called from amqp thread
   void consume_message( const AMQP::Message& message, const amqp_handler::delivery_tag_t& delivery_tag, bool redelivered ) {
      try {
         fc::datastream<const char*> ds( message.body(), message.bodySize() );
         fc::unsigned_int which;
         fc::raw::unpack(ds, which);
         std::string block_uuid_rk = message.headers().get("block-uuid-msg");
         if( which == fc::unsigned_int(fc::get_index<transaction_msg, chain::packed_transaction_v0>()) ) {
            chain::packed_transaction_v0 v0;
            fc::raw::unpack(ds, v0);
            auto ptr = std::make_shared<chain::packed_transaction>( std::move( v0 ), true );
            handle_message( delivery_tag, message.replyTo(), message.correlationID(), std::move(block_uuid_rk), std::move( ptr ) );
         } else if ( which == fc::unsigned_int(fc::get_index<transaction_msg, chain::packed_transaction>()) ) {
            auto ptr = std::make_shared<chain::packed_transaction>();
            fc::raw::unpack(ds, *ptr);
            handle_message( delivery_tag, message.replyTo(), message.correlationID(), std::move(block_uuid_rk), std::move( ptr ) );
         } else {
            FC_THROW_EXCEPTION( fc::out_of_range_exception, "Invalid which ${w} for consume of transaction_type message", ("w", which) );
         }
         if( acked == ack_mode::received ) {
            amqp_trx->ack( delivery_tag );
         }
         return;
      } FC_LOG_AND_DROP()

      amqp_trx->reject( delivery_tag, false, false );
   }

   void on_block_start( uint32_t bn ) {
      if (!prod_plugin->paused() || allow_speculative_execution) {
         if (!started_consuming) {
            ilog("Starting consuming amqp messages during on_block_start");
            amqp_trx->start_consume(amqp_trx_queue,
               [&]( const AMQP::Message& message, const amqp_handler::delivery_tag_t& delivery_tag, bool redelivered ) {
                  if( app().is_quiting() ) return; // leave non-ack
                  consume_message( message, delivery_tag, redelivered );
            }, true);
            started_consuming = true;
         }

         block_uuid = boost::uuids::to_string( boost::uuids::random_generator()() );
         tracked_block_uuid_rks.clear();
         trx_queue_ptr->on_block_start();
      } else {
         if( prod_plugin->paused() && started_consuming ) {
            ilog("Stopping consuming amqp messages during on_block_start");
            amqp_trx->stop_consume([](const std::string& consumer_tag){
               dlog("Stopped consuming from amqp tag: {t}", ("t", consumer_tag));
            });
            started_consuming = false;
            const bool clear = true;
            amqp_handler::delivery_tag_t delivery_tag = 0;
            trx_queue_ptr->for_each_delivery_tag([&](const amqp_handler::delivery_tag_t& i_delivery_tag){
               delivery_tag = i_delivery_tag;
            }, clear);
            const bool multiple = true;
            const bool requeue = true;
            if(delivery_tag != 0) amqp_trx->reject(delivery_tag, multiple, requeue);
         }
      }

   }

   void on_block_abort( uint32_t bn ) {
      trx_queue_ptr->on_block_stop();
      tracked_block_uuid_rks.clear();
      block_uuid.clear();
   }

   void on_accepted_block( const chain::block_state_ptr& bsp ) {
      if( acked == ack_mode::in_block ) {
         const auto& entry = tracked_delivery_tags.find( bsp->block_num );
         if( entry != tracked_delivery_tags.end() ) {
            amqp_trx->ack( entry->second, true );
            tracked_delivery_tags.erase( entry );
         }
      }
      trx_queue_ptr->on_block_stop();
      for( auto& e : tracked_block_uuid_rks ) {
         trace_plug.publish_block_uuid( std::move( e ), block_uuid, bsp->id );
      }
      tracked_block_uuid_rks.clear();
      block_uuid.clear();
   }

private:

   // called from amqp thread
   void handle_message( const amqp_handler::delivery_tag_t& delivery_tag,
                        const std::string& reply_to,
                        const std::string& correlation_id,
                        std::string block_uuid_rk,
                        chain::packed_transaction_ptr trx ) {
      static_assert(std::is_same_v<amqp_handler::delivery_tag_t, uint64_t>, "fifo_trx_processing_queue assumes delivery_tag is an uint64_t");
      const auto& tid = trx->id();
      dlog( "received packed_transaction {id}", ("id", tid) );

      auto trx_trace = fc_create_trace_with_id("Transaction", tid);
      auto trx_span = fc_create_span(trx_trace, "AMQP Received");
      fc_add_tag(trx_span, "trx_id", tid);

      trx_queue_ptr->push( trx, delivery_tag,
                [my=shared_from_this(), token=fc_get_token(trx_trace),
                 delivery_tag, reply_to, correlation_id, block_uuid_rk=std::move(block_uuid_rk), trx]
                (const std::variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) mutable {
            auto trx_span = fc_create_span_from_token(token, "Processed");
            fc_add_tag(trx_span, "trx_id", trx->id());

            // publish to trace plugin as exceptions are not reported via controller signal applied_transaction
            if( std::holds_alternative<chain::exception_ptr>(result) ) {
               auto& eptr = std::get<chain::exception_ptr>(result);
               fc_add_tag(trx_span, "error", eptr->to_string());
               dlog( "accept_transaction {id} exception: {e}", ("id", trx->id())("e", eptr->to_string()) );
               if( my->acked == ack_mode::executed || my->acked == ack_mode::in_block ) { // ack immediately on failure
                  my->amqp_trx->ack( delivery_tag );
               }
               if( !reply_to.empty() ) {
                  my->trace_plug.publish_error( std::move(reply_to), std::move(correlation_id), eptr->code(), eptr->to_string() );
               }
            } else {
               auto& trace = std::get<chain::transaction_trace_ptr>(result);
               fc_add_tag(trx_span, "block_num", trace->block_num);
               fc_add_tag(trx_span, "block_time", trace->block_time.to_time_point());
               fc_add_tag(trx_span, "elapsed", trace->elapsed.count());
               if( trace->receipt ) {
                  fc_add_tag(trx_span, "status", std::string(trace->receipt->status));
               }
               if( trace->except ) {
                  fc_add_tag(trx_span, "error", trace->except->to_string());
                  dlog( "accept_transaction {id} exception: {e}", ("id", trx->id())("e", trace->except->to_string()) );
                  if( my->acked == ack_mode::executed || my->acked == ack_mode::in_block ) { // ack immediately on failure
                     my->amqp_trx->ack( delivery_tag );
                  }
               } else {
                  dlog( "accept_transaction {id}", ("id", trx->id()) );
                  if( my->acked == ack_mode::executed ) {
                     my->amqp_trx->ack( delivery_tag );
                  } else if( my->acked == ack_mode::in_block ) {
                     my->tracked_delivery_tags[trace->block_num] = delivery_tag;
                  }
                  if( !block_uuid_rk.empty() ) {
                     my->tracked_block_uuid_rks.emplace( std::move( block_uuid_rk ) );
                  }
               }
               if( !reply_to.empty() ) {
                  my->trace_plug.publish_result( std::move(reply_to), std::move(correlation_id), my->block_uuid, trx, trace );
               }
            }
         } );
   }
};

amqp_trx_plugin::amqp_trx_plugin()
: my(std::make_shared<amqp_trx_plugin_impl>()) {
   app().register_config_type<ack_mode>();
   app().register_config_type<amqp_trace_plugin_impl::reliable_mode>();
}

amqp_trx_plugin::~amqp_trx_plugin() {}

void amqp_trx_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("amqp-trx-address", bpo::value<std::string>()->default_value(std::getenv(EOSIO_AMQP_ADDRESS_ENV_VAR) ? std::getenv(EOSIO_AMQP_ADDRESS_ENV_VAR) : ""),
      "AMQP address: Format: amqp://USER:PASSWORD@ADDRESS:PORT\n"
      "Will consume from amqp-trx-queue-name (amqp-trx-queue-name) queue.\n"
      "If --amqp-trx-address is not specified, will use the value from the environment variable "
      EOSIO_AMQP_ADDRESS_ENV_VAR
      ".");
   op("amqp-trx-queue-name", bpo::value<std::string>()->default_value("trx"),
      "AMQP queue to consume transactions from, must already exist.");
   op("amqp-trx-queue-size", bpo::value<uint32_t>()->default_value(my->trx_processing_queue_size),
      "The maximum number of transactions to pull from the AMQP queue at any given time.");
   op("amqp-trx-retry-timeout-us", bpo::value<uint32_t>()->default_value(my->trx_retry_timeout_us),
      "Time in microseconds to continue to retry a connection to AMQP when connection is loss or startup.");
   op("amqp-trx-retry-interval-us", bpo::value<uint32_t>()->default_value(my->trx_retry_interval_us),
      "When connection is lost to amqp-trx-queue-name, interval time in microseconds before retrying connection.");
   op("amqp-trx-speculative-execution", bpo::bool_switch()->default_value(false),
      "Allow non-ordered speculative execution of transactions");
   op("amqp-trx-ack-mode", bpo::value<ack_mode>()->default_value(ack_mode::in_block),
      "AMQP ack when 'received' from AMQP, when 'executed', or when 'in_block' is produced that contains trx.\n"
      "Options: received, executed, in_block");
   op("amqp-trx-trace-reliable-mode", bpo::value<amqp_trace_plugin_impl::reliable_mode>()->default_value(amqp_trace_plugin_impl::reliable_mode::queue),
      "If AMQP reply-to header is set, transaction trace is sent to default exchange with routing key of the reply-to header.\n"
      "If AMQP reply-to header is not set, then transaction trace is discarded.\n"
      "Reliable mode 'exit', exit application on any AMQP publish error.\n"
      "Reliable mode 'queue', queue transaction traces to send to AMQP on connection establishment.\n"
      "Reliable mode 'log', log an error and drop message when unable to directly publish to AMQP.");
}

void amqp_trx_plugin::plugin_initialize(const variables_map& options) {
   try {
      my->chain_plug = app().find_plugin<chain_plugin>();
      my->prod_plugin = app().find_plugin<producer_plugin>();
      EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, "chain_plugin required" );

      EOS_ASSERT( options.count("amqp-trx-address"), chain::plugin_config_exception, "amqp-trx-address required" );
      my->amqp_trx_address = options.at("amqp-trx-address").as<std::string>();

      my->amqp_trx_queue = options.at("amqp-trx-queue-name").as<std::string>();
      EOS_ASSERT( !my->amqp_trx_queue.empty(), chain::plugin_config_exception, "amqp-trx-queue-name required" );

      my->acked = options.at("amqp-trx-ack-mode").as<ack_mode>();

      my->trx_processing_queue_size = options.at("amqp-trx-queue-size").as<uint32_t>();
      my->trx_retry_timeout_us = options.at("amqp-trx-retry-timeout-us").as<uint32_t>();
      my->trx_retry_interval_us = options.at("amqp-trx-retry-interval-us").as<uint32_t>();
      my->allow_speculative_execution = options.at("amqp-trx-speculative-execution").as<bool>();

      EOS_ASSERT( my->acked != ack_mode::in_block || !my->allow_speculative_execution, chain::plugin_config_exception,
                  "amqp-trx-ack-mode = in_block not supported with amqp-trx-speculative-execution" );

      my->trace_plug.amqp_trace_address = my->amqp_trx_address;
      my->trace_plug.amqp_trace_queue_name = ""; // not used, reply-to is used for each message
      my->trace_plug.amqp_trace_exchange = ""; // default exchange, reply-to used for routing-key
      my->trace_plug.pub_reliable_mode = options.at("amqp-trx-trace-reliable-mode").as<amqp_trace_plugin_impl::reliable_mode>();

      my->chain_plug->enable_accept_transactions();
   }
   FC_LOG_AND_RETHROW()
}

void amqp_trx_plugin::plugin_startup() {
   handle_sighup();

   ilog( "Starting amqp_trx_plugin" );

   EOS_ASSERT( my->prod_plugin, chain::plugin_config_exception, "producer_plugin required" ); // should not be possible
   EOS_ASSERT( my->allow_speculative_execution || my->prod_plugin->has_producers(), chain::plugin_config_exception,
               "Must be a producer to run without amqp-trx-speculative-execution" );

   auto& chain = my->chain_plug->chain();
   my->trx_queue_ptr =
         std::make_shared<fifo_trx_processing_queue<producer_plugin>>( chain.get_chain_id(),
                                                                       chain.configured_subjective_signature_length_limit(),
                                                                       my->allow_speculative_execution,
                                                                       chain.get_thread_pool(),
                                                                       my->prod_plugin,
                                                                       my->trx_processing_queue_size );

   const boost::filesystem::path trace_data_dir_path = appbase::app().data_dir() / "amqp_trx_plugin";
   const boost::filesystem::path trace_data_file_path = trace_data_dir_path / "trxtrace.bin";
   if( my->trace_plug.pub_reliable_mode != amqp_trace_plugin_impl::reliable_mode::queue ) {
      EOS_ASSERT( !fc::exists( trace_data_file_path ), chain::plugin_config_exception,
                  "Existing queue file when amqp-trx-trace-reliable-mode != 'queue': ${f}",
                  ("f", trace_data_file_path.generic_string()) );
   } else if( auto resmon_plugin = app().find_plugin<resource_monitor_plugin>() ) {
      resmon_plugin->monitor_directory( trace_data_dir_path );
   }

   my->trace_plug.amqp_trace.emplace( my->trace_plug.amqp_trace_address, my->trace_plug.amqp_trace_exchange,
                                      my->trace_plug.amqp_trace_queue_name, trace_data_file_path,
                                      []( const std::string& err ) {
                                         elog( "AMQP fatal error: {e}", ("e", err) );
                                         appbase::app().quit();
                                      } );

   my->block_start_connection.emplace(
         chain.block_start.connect( [this]( uint32_t bn ) { my->on_block_start( bn ); } ) );
   my->block_abort_connection.emplace(
         chain.block_abort.connect( [this]( uint32_t bn ) { my->on_block_abort( bn ); } ) );
   my->accepted_block_connection.emplace(
         chain.accepted_block.connect( [this]( const auto& bsp ) { my->on_accepted_block( bsp ); } ) );

   my->trx_queue_ptr->run();

   my->amqp_trx.emplace( my->amqp_trx_address,
                         fc::microseconds(my->trx_retry_timeout_us),
                         fc::microseconds(my->trx_retry_interval_us),
                         []( const std::string& err ) {
                            elog( "amqp error: {e}", ("e", err) );
                            app().quit();
                         }
   );

   if (!my->prod_plugin->paused() || my->allow_speculative_execution) {
      ilog("Starting amqp consumption at startup.");
      my->amqp_trx->start_consume(my->amqp_trx_queue,
         [&]( const AMQP::Message& message, const amqp_handler::delivery_tag_t& delivery_tag, bool redelivered ) {
            if( app().is_quiting() ) return; // leave non-ack
            my->consume_message( message, delivery_tag, redelivered );
         }, true);
      my->started_consuming = true;
   }
}

void amqp_trx_plugin::plugin_shutdown() {
   try {
      dlog( "shutdown.." );

      if( my->trx_queue_ptr ) {
         // Need to stop processing from queue since amqp_handler can be paused waiting on queue to empty.
         // Without this it is possible for the amqp_trx->stop() to hang forever waiting on the trx_queue.
         my->trx_queue_ptr->signal_stop();
      }

      if( my->amqp_trx ) {
         my->amqp_trx->stop();
      }

      dlog( "stopping fifo queue" );
      if( my->trx_queue_ptr ) {
         my->trx_queue_ptr->stop();
      }

      dlog( "exit amqp_trx_plugin" );
   }
   FC_LOG_AND_DROP()
}

void amqp_trx_plugin::handle_sighup() {
}

} // namespace eosio
