#include <eosio/rabbitmq_trx_plugin/rabbitmq_trx_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/thread_utils.hpp>

#include "amqpcpp.h"
#include "amqpcpp/libboostasio.h"
#include "amqpcpp/linux_tcp.h"

namespace {

static appbase::abstract_plugin& rabbitmq_trx_plugin_ = appbase::app().register_plugin<eosio::rabbitmq_trx_plugin>();

const fc::string logger_name{"rabbitmq_trx"};
fc::logger logger;

} // anonymous

namespace eosio {

class rabbitmq_handler : public AMQP::LibBoostAsioHandler {
public:
   explicit rabbitmq_handler(boost::asio::io_service& io_service) : AMQP::LibBoostAsioHandler(io_service) {}

   void onError(AMQP::TcpConnection* connection, const char* message) override {
      throw std::runtime_error("rabbitmq connection failed: " + std::string(message));
   }

   uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval) override {
      return 0; // disable heartbeats
   }

};

class rabbitmq {
   std::unique_ptr<rabbitmq_handler>    handler_;
   std::unique_ptr<AMQP::TcpConnection> connection_;
   std::unique_ptr<AMQP::TcpChannel>    channel_;
   std::string                          name_;

public:
   rabbitmq(boost::asio::io_service& io_service, const std::string& address, std::string name)
         : name_(std::move(name)) {
      AMQP::Address amqp_address(address);
      fc_ilog( logger, "Connecting to RabbitMQ address ${a} - Queue: ${q}...", ("a", std::string(amqp_address))("q", name_) );

      handler_    = std::make_unique<rabbitmq_handler>(io_service);
      connection_ = std::make_unique<AMQP::TcpConnection>(handler_.get(), amqp_address);
      channel_    = std::make_unique<AMQP::TcpChannel>(connection_.get());
      declare_queue();
   }

   AMQP::TcpChannel& get_channel() { return *channel_; }

   void publish(const std::string& correlation_id, const char* data, size_t data_size) {
      AMQP::Envelope env( data, data_size );
      env.setCorrelationID( correlation_id );
      channel_->publish("", name_, env, 0);
   }
   auto& consume() { return channel_->consume(name_); }

private:
   void declare_queue() {
      auto& queue = channel_->declareQueue(name_, AMQP::durable);
      queue.onSuccess([](const std::string& name, uint32_t messagecount, uint32_t consumercount) {
         fc_ilog(logger, "RabbitMQ Connected Successfully!\n Queue ${q} - Messages: ${mc} - Consumers: ${cc}",
              ("q", name)("mc", messagecount)("cc", consumercount));
      });
      queue.onError([](const char* error_message) {
         std::string err = "RabbitMQ Queue error: " + std::string(error_message);
         fc_wlog( logger, err );
         throw std::runtime_error( err );
      });
   }
};

constexpr auto     def_max_trx_in_progress_size = 100*1024*1024; // 100 MB

struct rabbitmq_trx_plugin_impl : std::enable_shared_from_this<rabbitmq_trx_plugin_impl> {

   chain_plugin* chain_plug = nullptr;
   // use thread pool even though only one thread currently since it provides simple interface for ioc
   std::optional<eosio::chain::named_thread_pool> thread_pool;
   std::optional<rabbitmq> rabbitmq_trx;
   std::optional<rabbitmq> rabbitmq_trace;

   std::string rabbitmq_trx_address;
   bool rabbitmq_publish_all_traces = false;
   std::atomic<uint32_t>  trx_in_progress_size{0};

   // called from rabbitmq thread
   bool consume_message( const char* buf, size_t s ) {
      try {
         fc::datastream<const char*> ds( buf, s );
         fc::unsigned_int which{};
         fc::raw::unpack( ds, which );
         switch( which ) {
            case transaction_msg::tag<chain::packed_transaction_v0>::value: {
               chain::packed_transaction_v0 v0;
               fc::raw::unpack( ds, v0 );
               auto ptr = std::make_shared<chain::packed_transaction>( std::move( v0 ), true );
               handle_message( std::move( ptr ) );
               break;
            }
            case transaction_msg::tag<chain::packed_transaction>::value: {
               auto ptr = std::make_shared<chain::packed_transaction>();
               fc::raw::unpack( ds, *ptr );
               handle_message( std::move( ptr ) );
               break;
            }
            default:
               FC_THROW_EXCEPTION( fc::out_of_range_exception, "Invalid which ${w} for consume of transaction_type message", ("w", which) );
         }
         return true;
      } FC_LOG_AND_DROP()
      return false;
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
         rabbitmq_trace->publish( tid.str(), buf.data(), buf.size() );
         return;
      }

      trx_in_progress_size += trx->get_estimated_size();
      app().post( priority::medium_low, [my=shared_from_this(), trx{std::move(trx)}]() {
         my->chain_plug->accept_transaction( trx,
            [my, trx](const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) mutable {
               boost::asio::post( my->thread_pool->get_executor(), [my, trx = std::move( trx ), result=result]() {
                  my->publish_result( trx, result );
               } );
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
            rabbitmq_trace->publish( trx->id(), buf.data(), buf.size() );

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
            rabbitmq_trace->publish( trx->id(), buf.data(), buf.size() );
         }
      } FC_LOG_AND_DROP()
      trx_in_progress_size -= trx->get_estimated_size();
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
   op("rabbitmq-publish-all-traces", bpo::bool_switch()->default_value(false),
      "If specified then all traces will be published; otherwise only traces for consumed 'trx' queue transactions.");
}

void rabbitmq_trx_plugin::plugin_initialize(const variables_map& options) {
   try {
      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT( my->chain_plug, chain::missing_chain_plugin_exception, "chain_plugin required" );

      EOS_ASSERT( options.count("rabbitmq-trx-address"), chain::plugin_config_exception, "rabbitmq-trx-address required" );
      my->rabbitmq_trx_address = options.at("rabbitmq-trx-address").as<std::string>();
      my->rabbitmq_publish_all_traces = options.at("rabbitmq-publish-all-traces").as<bool>();
   }
   FC_LOG_AND_RETHROW()
}

void rabbitmq_trx_plugin::plugin_startup() {
   handle_sighup();
   try {

      my->thread_pool.emplace( "rabmq", 1 );

      my->rabbitmq_trx.emplace( my->thread_pool->get_executor(), my->rabbitmq_trx_address, "trx" );
      my->rabbitmq_trace.emplace( my->thread_pool->get_executor(), my->rabbitmq_trx_address, "trace" );

      auto& consumer = my->rabbitmq_trx->consume();
      consumer.onSuccess( []( const std::string& consumer_tag ) {
         fc_dlog( logger, "consume started: ${tag}", ("tag", consumer_tag) );
      } );
      consumer.onError( []( const char* message ) {
         fc_wlog( logger, "consume failed: ${e}", ("e", message) );
      } );
      consumer.onReceived(
            [&channel = my->rabbitmq_trx->get_channel(), my=my]
            (const AMQP::Message& message, uint64_t delivery_tag, bool redelivered) {
         if( my->consume_message( message.body(), message.bodySize() ) ) {
            channel.ack( delivery_tag );
         } else {
            channel.reject( delivery_tag );
         }
      } );

   } catch( ... ) {
      // always want plugin_shutdown even on exception
      plugin_shutdown();
      throw;
   }
}

void rabbitmq_trx_plugin::plugin_shutdown() {
   try {
      fc_dlog( logger, "shutdown.." );
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
