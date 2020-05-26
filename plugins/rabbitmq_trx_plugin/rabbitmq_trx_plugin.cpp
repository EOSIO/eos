#include <eosio/rabbitmq_trx_plugin/rabbitmq_trx_plugin.hpp>
#include <eosio/rabbitmq_trx_plugin/rabbitmq.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/thread_utils.hpp>

#include <boost/signals2/connection.hpp>

namespace {

static appbase::abstract_plugin& rabbitmq_trx_plugin_ = appbase::app().register_plugin<eosio::rabbitmq_trx_plugin>();

const fc::string logger_name{"rabbitmq_trx"};
fc::logger logger;

constexpr auto def_max_trx_in_progress_size = 100*1024*1024; // 100 MB

} // anonymous

namespace eosio {

using boost::signals2::scoped_connection;

struct rabbitmq_trx_plugin_impl : std::enable_shared_from_this<rabbitmq_trx_plugin_impl> {

   chain_plugin* chain_plug = nullptr;
   // use thread pool even though only one thread currently since it provides simple interface for ioc
   std::optional<eosio::chain::named_thread_pool> thread_pool;
   std::optional<rabbitmq> rabbitmq_trx;
   std::optional<rabbitmq> rabbitmq_trace;
   std::optional<scoped_connection> applied_transaction_connection;

   std::string rabbitmq_trx_address;
   std::string rabbitmq_exchange;
   bool rabbitmq_publish_all_traces = false;
   std::atomic<uint32_t>  trx_in_progress_size{0};

   // called from rabbitmq thread
   bool consume_message( const char* buf, size_t s ) {
      try {
         fc::datastream<const char*> ds( buf, s );
         transaction_msg msg;
         fc::raw::unpack(ds, msg);
         if( msg.contains<chain::packed_transaction_v0>() ) {
            auto ptr = std::make_shared<chain::packed_transaction>( std::move( msg.get<chain::packed_transaction_v0>() ), true );
            handle_message( std::move( ptr ) );
         } else if( msg.contains<chain::packed_transaction>() ) {
            auto ptr = std::make_shared<chain::packed_transaction>( std::move( msg.get<chain::packed_transaction>() ) );
            handle_message( std::move( ptr ) );
         } else {
            FC_THROW_EXCEPTION( fc::out_of_range_exception, "Invalid which ${w} for consume of transaction_type message",
                                ("w", msg.which()) );
         }
         return true;
      } FC_LOG_AND_DROP()
      return false;
   }

   // only called if rabbitmq-publish-all-traces=true
   // called on application thread
   void on_applied_transaction(const chain::transaction_trace_ptr& trace, const chain::packed_transaction_ptr& t) {
      try {
         boost::asio::post( thread_pool->get_executor(), [my=shared_from_this(), trace, t]() {
            my->publish_result( t, trace );
         } );
      } FC_LOG_AND_DROP()
   }

private:

   // called from rabbitmq thread
   void handle_message( chain::packed_transaction_ptr trx ) {
      const auto& tid = trx->id();
      fc_dlog( logger, "received packed_transaction ${id}", ("id", tid) );

      auto trx_in_progress = trx_in_progress_size.load();
      if( trx_in_progress > def_max_trx_in_progress_size ) {
         fc_wlog( logger, "Dropping trx, too many trx in progress ${s} bytes", ("s", trx_in_progress) );
         transaction_trace_msg msg{ transaction_trace_exception{ chain::tx_resource_exhaustion::code_enum::code_value } };
         msg.get<transaction_trace_exception>().error_message =
               "Dropped trx, too many trx in progress " + std::to_string( trx_in_progress ) + " bytes";
         auto buf = fc::raw::pack( msg );
         rabbitmq_trace->publish( rabbitmq_exchange, tid.str(), buf.data(), buf.size() );
         return;
      }

      trx_in_progress_size += trx->get_estimated_size();
      app().post( priority::medium_low, [my=shared_from_this(), trx{std::move(trx)}]() {
         my->chain_plug->accept_transaction( trx,
            [my, trx](const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) mutable {
               // if rabbitmq-publish-all-traces=true and a transaction trace then no need to publish here as already published
               if( my->rabbitmq_publish_all_traces && result.contains<chain::transaction_trace_ptr>() ) {
                  my->trx_in_progress_size -= trx->get_estimated_size();
               } else {
                  boost::asio::post( my->thread_pool->get_executor(), [my, trx = std::move( trx ), result = result]() {
                     my->publish_result( trx, result );
                     my->trx_in_progress_size -= trx->get_estimated_size();
                  } );
               }
            } );
         } );
   }

   // called from rabbitmq thread
   void publish_result( const chain::packed_transaction_ptr& trx,
                        const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result ) {

      try {
         if( result.contains<fc::exception_ptr>() ) {
            auto& ex = *result.get<fc::exception_ptr>();
            std::string err = ex.to_string();
            fc_dlog( logger, "bad packed_transaction : ${e}", ("e", err) );
            transaction_trace_exception tex{ ex.code() };
            fc::unsigned_int which = transaction_trace_msg::tag<transaction_trace_exception>::value;
            // TODO; use fc::datastream<std::vector<char>> when available
            uint32_t payload_size = fc::raw::pack_size( which );
            payload_size += fc::raw::pack_size( tex.error_code );
            payload_size += fc::raw::pack_size( err );
            std::vector<char> buf( payload_size );
            fc::datastream<char*> ds( buf.data(), payload_size );
            fc::raw::pack( ds, which );
            fc::raw::pack( ds, tex.error_code );
            fc::raw::pack( ds, err );
            rabbitmq_trace->publish( rabbitmq_exchange, trx->id(), buf.data(), buf.size() );

         } else {
            const auto& trace = result.get<chain::transaction_trace_ptr>();
            if( !trace->except ) {
               fc_dlog( logger, "chain accepted transaction, bcast ${id}", ("id", trace->id) );
            } else {
               fc_dlog( logger, "trace except : ${m}", ("m", trace->except->to_string()) );
            }
            fc::unsigned_int which = transaction_trace_msg::tag<chain::transaction_trace>::value;
            // TODO; use fc::datastream<std::vector<char>> when available
            uint32_t payload_size = fc::raw::pack_size( which );
            payload_size += fc::raw::pack_size( *trace );
            std::vector<char> buf( payload_size );
            fc::datastream<char*> ds( buf.data(), payload_size );
            fc::raw::pack( ds, which );
            fc::raw::pack( ds, *trace );
            rabbitmq_trace->publish( rabbitmq_exchange, trx->id(), buf.data(), buf.size() );
         }
      } FC_LOG_AND_DROP()
   }

};

rabbitmq_trx_plugin::rabbitmq_trx_plugin()
: my(std::make_shared<rabbitmq_trx_plugin_impl>()) {}

rabbitmq_trx_plugin::~rabbitmq_trx_plugin() {}

void rabbitmq_trx_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("rabbitmq-trx-address", bpo::value<std::string>(),
      "RabbitMQ address: Format: amqp://USER:PASSWORD@ADDRESS:PORT\n"
      "Will consume from 'trx' queue and publish to 'trace' queue.");
   op("rabbitmq-trx-exchange", bpo::value<std::string>()->default_value(""),
      "Existing RabbitMQ exchange to send transaction trace messages.");
   op("rabbitmq-publish-all-traces", bpo::bool_switch()->default_value(false),
      "If specified then all traces will be published; otherwise only traces for consumed 'trx' queue transactions.");
}

void rabbitmq_trx_plugin::plugin_initialize(const variables_map& options) {
   try {
      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, "chain_plugin required" );

      EOS_ASSERT( options.count("rabbitmq-trx-address"), chain::plugin_config_exception, "rabbitmq-trx-address required" );
      my->rabbitmq_trx_address = options.at("rabbitmq-trx-address").as<std::string>();
      my->rabbitmq_exchange = options.at("rabbitmq-trx-exchange").as<std::string>();
      my->rabbitmq_publish_all_traces = options.at("rabbitmq-publish-all-traces").as<bool>();
   }
   FC_LOG_AND_RETHROW()
}

void rabbitmq_trx_plugin::plugin_startup() {
   handle_sighup();
   try {

      my->thread_pool.emplace( "rabmq", 1 );

      my->rabbitmq_trx.emplace( logger, my->thread_pool->get_executor(), my->rabbitmq_trx_address, "trx" );
      my->rabbitmq_trace.emplace( logger, my->thread_pool->get_executor(), my->rabbitmq_trx_address, "trace" );

      auto& consumer = my->rabbitmq_trx->consume();
      consumer.onSuccess( []( const std::string& consumer_tag ) {
         fc_dlog( logger, "consume started: ${tag}", ("tag", consumer_tag) );
      } );
      consumer.onError( []( const char* message ) {
         fc_wlog( logger, "consume failed: ${e}", ("e", message) );
      } );
      consumer.onReceived( [my=my](const AMQP::Message& message, uint64_t delivery_tag, bool redelivered) {
         if( my->consume_message( message.body(), message.bodySize() ) ) {
            my->rabbitmq_trx->ack( delivery_tag );
         } else {
            my->rabbitmq_trx->reject( delivery_tag );
         }
      } );

      if( my->rabbitmq_publish_all_traces ) {
         auto& chain_plug = app().get_plugin<chain_plugin>();
         my->applied_transaction_connection.emplace(
               chain_plug.chain().applied_transaction.connect(
                     [&]( std::tuple<const chain::transaction_trace_ptr&, const chain::packed_transaction_ptr&> t ) {
                        my->on_applied_transaction( std::get<0>( t ), std::get<1>( t ) );
                     } ) );
      }

   } catch( ... ) {
      // always want plugin_shutdown even on exception
      plugin_shutdown();
      throw;
   }
}

void rabbitmq_trx_plugin::plugin_shutdown() {
   try {
      fc_dlog( logger, "shutdown.." );
      my->applied_transaction_connection.reset();
      if( my->thread_pool ) {
         my->thread_pool->stop();
      }
      app().post( priority::lowest, [me = my](){} ); // keep my pointer alive until queue is drained
      fc_dlog( logger, "exit shutdown" );
   }
   FC_CAPTURE_AND_RETHROW()
}

void rabbitmq_trx_plugin::handle_sighup() {
   fc::logger::update( logger_name, logger );
}

} // namespace eosio
