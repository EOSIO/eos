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

using transaction_type = fc::static_variant<chain::packed_transaction_v0, chain::packed_transaction>;

class rabbitmq_handler : public AMQP::LibBoostAsioHandler {
public:
   explicit rabbitmq_handler(boost::asio::io_service& io_service) : AMQP::LibBoostAsioHandler(io_service) {}

   void onError(AMQP::TcpConnection* connection, const char* message) override {
      throw std::runtime_error("rabbitmq connection failed: " + std::string(message));
   }

   uint16_t onNegotiate(AMQP::TcpConnection *connection, uint16_t interval) override {
      // disable heartbeats
      return 0;
   }

};

class rabbitmq {
   std::unique_ptr<rabbitmq_handler>    handler_;
   std::unique_ptr<AMQP::TcpConnection> connection_;
   std::unique_ptr<AMQP::TcpChannel>    channel_;
   std::string                          name_;

public:
   rabbitmq(boost::asio::io_service& io_service, std::string address, std::string name)
         : name_(std::move(name)) {
      AMQP::Address amqp_address(address);
      fc_ilog( logger, "Connecting to RabbitMQ address ${a} - Queue: ${q}...", ("a", std::string(amqp_address))("q", name_) );

      handler_    = std::make_unique<rabbitmq_handler>(io_service);
      connection_ = std::make_unique<AMQP::TcpConnection>(handler_.get(), amqp_address);
      channel_    = std::make_unique<AMQP::TcpChannel>(connection_.get());
      declare_queue();
   }

   AMQP::TcpChannel& get_channel() { return *channel_; }

   void publish(const char* data, size_t data_size) { channel_->publish("", name_, data, data_size, 0); }
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
   std::optional<rabbitmq> trx_rabbitmq;
   std::optional<rabbitmq> trace_rabbitmq;

   std::string rabbitmq_trx_address;
   bool rabbitmq_publish_all_traces = false;
   std::atomic<uint32_t>  trx_in_progress_size{0};

   bool consume_message( const char* buf, size_t s ) {
      try {
         fc::datastream<const char*> ds( buf, s );
         fc::unsigned_int which{};
         fc::raw::unpack( ds, which );
         switch( which ) {
            case transaction_type::tag<chain::packed_transaction_v0>::value: {
               chain::packed_transaction_v0 v0;
               fc::raw::unpack( ds, v0 );
               handle_message( v0 );
               break;
            }
            case transaction_type::tag<chain::packed_transaction>::value: {
               auto ptr = std::make_shared<chain::packed_transaction>();
               fc::raw::unpack( ds, *ptr );
               handle_message( std::move( ptr ) );
               break;
            }
            default:
               FC_THROW_EXCEPTION( fc::out_of_range_exception, "Invalid which ${w} for consume of transaction_type message", ("w", which) );
         }
         transaction_type msg = fc::raw::unpack<transaction_type>( buf, s );
         if( msg.contains<chain::packed_transaction_v0>() ) {
            auto& pv0 = msg.get<chain::packed_transaction_v0>();
         }
         return true;
      } FC_LOG_AND_DROP()
      return false;
   }

   void handle_message( const chain::packed_transaction_v0& trx ) {

   }

   void handle_message( chain::packed_transaction_ptr trx ) {
      const auto& tid = trx->id();
      fc_dlog( logger, "received packed_transaction ${id}", ("id", tid) );

      trx_in_progress_size += trx->get_estimated_size();
      if( trx_in_progress_size > def_max_trx_in_progress_size ) {
         fc_wlog( logger, "Dropping trx, too many trx in progress ${s} bytes", ("s", trx_in_progress_size.load()) );
         // TODO: send error out publish trace channel
         return;
      }
      app().post( priority::medium_low, [my=shared_from_this(), trx{std::move(trx)}]() {
         my->chain_plug->accept_transaction( trx,
            [my, trx](const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) mutable {
         // next (this lambda) called from application thread
         if (result.contains<fc::exception_ptr>()) {
            fc_dlog( logger, "bad packed_transaction : ${m}", ("m", result.get<fc::exception_ptr>()->what()) );
         } else {
            const chain::transaction_trace_ptr& trace = result.get<chain::transaction_trace_ptr>();
            if( !trace->except ) {
               fc_dlog( logger, "chain accepted transaction, bcast ${id}", ("id", trace->id) );
            } else {
               fc_elog( logger, "bad packed_transaction : ${m}", ("m", trace->except->what()));
            }
         }
         my->trx_in_progress_size -= trx->get_estimated_size();
        });
      });
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

      my->trx_rabbitmq.emplace( my->thread_pool->get_executor(), my->rabbitmq_trx_address, "trx" );
      my->trace_rabbitmq.emplace( my->thread_pool->get_executor(), my->rabbitmq_trx_address, "trace" );

      auto& consumer = my->trx_rabbitmq->consume();
      consumer.onSuccess( []( const std::string& consumer_tag ) {
         fc_dlog( logger, "consume started: ${tag}", ("tag", consumer_tag) );
      } );
      consumer.onError( []( const char* message ) {
         fc_wlog( logger, "consume failed: ${e}", ("e", message) );
      } );
      consumer.onReceived(
            [&channel = my->trx_rabbitmq->get_channel(), my=my]
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
