#pragma once

#include <eosio/amqp/util.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <amqpcpp/linux_tcp.h>


namespace eosio {

/// Designed to work with internal io_service running on a dedicated thread.
/// All publish methods can be called from any thread, but should be sync'ed with stop() & destructor.
/// Constructor, stop(), destructor should be called from same thread.
class amqp {
public:
   // called from amqp thread or calling thread, but not concurrently
   using on_error_t = std::function<void(const std::string& err)>;
   // delivery_tag type of consume, use for ack/reject
   using delivery_tag_t = uint64_t;
   // called from amqp thread on consume of message
   using on_consume_t = std::function<void(const delivery_tag_t& delivery_tag, const char* buf, size_t s)>;

   /// @param address AMQP address
   /// @param name AMQP routing key
   /// @param on_err callback for errors, called from amqp thread or caller thread, can be nullptr
   /// @param on_consume callback for consume on routing key name, called from amqp thread, null if no consume needed.
   ///        user required to ack/reject delivery_tag for each callback.
   amqp( const std::string& address, std::string name, on_error_t on_err, on_consume_t on_consume = nullptr )
         : amqp( address, std::move(name), "", "", std::move(on_err), std::move(on_consume))
   {}

   /// @param address AMQP address
   /// @param exchange_name AMQP exchange to send message to
   /// @param exchange_type AMQP exhcnage type
   /// @param on_err callback for errors, called from amqp thread or caller thread, can be nullptr
   /// @param on_consume callback for consume on routing key name, called from amqp thread, null if no consume needed.
   ///        user required to ack/reject delivery_tag for each callback.
   amqp( const std::string& address, std::string exchange_name, std::string exchange_type, on_error_t on_err, on_consume_t on_consume = nullptr )
         : amqp( address, "", std::move(exchange_name), std::move(exchange_type), std::move(on_err), std::move(on_consume))
   {}

   /// drain queue and stop thread
   ~amqp() {
      stop();
   }

   /// publish to AMQP address with routing key name
   void publish( std::string exchange, std::string correlation_id, std::vector<char> buf ) {
      boost::asio::post( *handler_->amqp_strand(),
            [my=this, exchange=std::move(exchange), cid=std::move(correlation_id), buf=std::move(buf)]() {
               AMQP::Envelope env( buf.data(), buf.size() );
               env.setCorrelationID( cid );
               my->channel_->publish( exchange, my->name_, env, 0 );
      } );
   }

   /// publish to AMQP calling f() -> std::vector<char> on amqp thread
   template<typename Func>
   void publish( std::string exchange, std::string correlation_id, Func f ) {
      boost::asio::post( *handler_->amqp_strand(),
            [my=this, exchange=std::move(exchange), cid=std::move(correlation_id), f=std::move(f)]() {
               std::vector<char> buf = f();
               AMQP::Envelope env( buf.data(), buf.size() );
               env.setCorrelationID( cid );
               my->channel_->publish( exchange, my->name_, env, 0 );
      } );
   }

   /// ack consume message
   void ack( const delivery_tag_t& delivery_tag ) {
      boost::asio::post( *handler_->amqp_strand(),
            [my = this, delivery_tag]() {
               try {
                  my->channel_->ack( delivery_tag );
               } FC_LOG_AND_DROP()
            } );
   }

   // reject consume message
   void reject( const delivery_tag_t& delivery_tag ) {
      boost::asio::post( *handler_->amqp_strand(),
            [my = this, delivery_tag]() {
               try {
                  my->channel_->reject( delivery_tag );
               } FC_LOG_AND_DROP()
            } );
   }

   /// Explicitly stop thread pool execution
   /// Not thread safe, call only once from constructor/destructor thread
   /// Do not call from lambda's passed to publish or constructor e.g. on_error
   void stop() {
      if( handler_ ) {
         // drain amqp queue
         std::promise<void> stop_promise;
         auto future = stop_promise.get_future();
         boost::asio::post( *handler_->amqp_strand(), [&]() {
            if( channel_ ) {
               auto& cb = channel_->close();
               cb.onFinalize( [&]() {
                  stop_promise.set_value();
               } );
            } else {
               stop_promise.set_value();
            }
         } );
         future.wait();

         thread_pool_.stop();
         handler_.reset();
      }
   }

private:

   amqp(const std::string& address, std::string name, std::string exchange_name, std::string exchange_type, on_error_t on_err, on_consume_t on_consume)
         : thread_pool_( "ampq", 1 ) // amqp is not thread safe, use only one thread
         , connected_future_( connected_.get_future() )
         , name_( std::move( name ) )
         , exchange_name_( std::move( exchange_name ) )
         , exchange_type_( std::move( exchange_type ) )
         , on_error_( std::move( on_err ) )
         , on_consume_( std::move( on_consume ) )
   {
      AMQP::Address amqp_address( address );
      ilog( "Connecting to AMQP address ${a} - Queue: ${q}...", ("a", amqp_address)("q", name_) );

      handler_ = std::make_unique<amqp_handler>( *this, thread_pool_.get_executor() );
      boost::asio::post( *handler_->amqp_strand(), [&]() {
         connection_ = std::make_unique<AMQP::TcpConnection>( handler_.get(), amqp_address );
      });
      wait();
   }

   // called from amqp thread
   void init( AMQP::TcpConnection* c ) {
      channel_ = std::make_unique<AMQP::TcpChannel>( c );
      if( !exchange_type_.empty() ) {
         auto type = AMQP::direct;
         if (exchange_type_ == "fanout") {
            type = AMQP::fanout;
         } else if (exchange_type_ != "direct") {
            on_error( "Unsupported exchange type: " + exchange_type_ );
            connected_.set_value();
            return;
         }

         auto& exchange = channel_->declareExchange( exchange_name_, type, AMQP::durable);
         exchange.onSuccess( [this]() {
            dlog( "AMQP declare exchange Successfully!\n Exchange ${e}", ("e", exchange_name_) );
            init_consume();
         } );
         exchange.onError([this](const char* error_message) {
            on_error( std::string("AMQP Queue error: ") + error_message );
            connected_.set_value();
         });
      } else {
         auto& queue = channel_->declareQueue( name_, AMQP::durable );
         queue.onSuccess( [&]( const std::string& name, uint32_t messagecount, uint32_t consumercount ) {
            dlog( "AMQP Connected Successfully!\n Queue ${q} - Messages: ${mc} - Consumers: ${cc}",
                  ("q", name)( "mc", messagecount )( "cc", consumercount ) );
            init_consume();
         } );
         queue.onError( [&]( const char* error_message ) {
            on_error( error_message );
            connected_.set_value();
         } );
      }
   }

   // called from amqp thread
   void init_consume() {
      if( !on_consume_ ) {
         connected_.set_value();
         return;
      }

      auto& consumer = channel_->consume( name_ );
      consumer.onSuccess( [&]( const std::string& consumer_tag ) {
         dlog( "consume started: ${tag}", ("tag", consumer_tag) );
         connected_.set_value();
      } );
      consumer.onError( [&]( const char* message ) {
         wlog( "consume failed: ${e}", ("e", message) );
         on_error( message );
         connected_.set_value();
      } );
      consumer.onReceived( [&](const AMQP::Message& message, const delivery_tag_t& delivery_tag, bool redelivered) {
         on_consume_( delivery_tag, message.body(), message.bodySize() );
      } );
   }

   // called from non-amqp thread
   void wait() {
      auto r = connected_future_.wait_for( std::chrono::seconds( 10 ) );
      if( r == std::future_status::timeout ) {
         on_error( "AMQP timeout declaring queue" );
      }
   }

   void on_error( const std::string& message ) {
      std::lock_guard<std::mutex> g(mtx_);
      if( on_error_ ) on_error_( message );
   }

   class amqp_handler : public AMQP::LibBoostAsioHandler {
   public:
      explicit amqp_handler( amqp& impl, boost::asio::io_service& io_service )
            : AMQP::LibBoostAsioHandler( io_service )
            , amqp_(impl)
      {}

      void onError( AMQP::TcpConnection* connection, const char* message ) override {
         elog( "amqp connection failed: ${m}", ("m", message) );
         amqp_.on_error( message );
      }

      uint16_t onNegotiate( AMQP::TcpConnection* connection, uint16_t interval ) override {
         return 0; // disable heartbeats
      }

      void onReady(AMQP::TcpConnection* c) override {
         amqp_.init( c );
      }

      strand_shared_ptr& amqp_strand() {
         return _strand;
      }

      amqp& amqp_;
   };

private:
   std::mutex mtx_;
   eosio::chain::named_thread_pool thread_pool_;
   std::unique_ptr<amqp_handler> handler_;
   std::unique_ptr<AMQP::TcpConnection> connection_;
   std::unique_ptr<AMQP::TcpChannel> channel_;
   std::promise<void> connected_;
   std::future<void> connected_future_;
   std::string name_;
   std::string exchange_name_;
   std::string exchange_type_;
   on_error_t on_error_;
   on_consume_t on_consume_;
};

} // namespace eosio
