#pragma once

#include <eosio/chain/thread_utils.hpp>
#include <fc/log/logger.hpp>

#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>
#include <amqpcpp/linux_tcp.h>


namespace eosio {

/// Designed to work with internal io_service running on a dedicated thread.
/// All publish methods can be called from any thread, but should be sync'ed with stop() & destructor.
/// Constructor, stop(), destructor should be called from same thread.
class amqp {
public:
   // called from amqp thread on errors
   using on_error_t = std::function<void(const std::string& err)>;
   // called from amqp thread on consume of message
   // return true for ack, false for reject
   using on_consume_t = std::function<bool(const char* buf, size_t s)>;

   /// @param address AMQP address
   /// @param name AMQP routing key
   /// @param on_err callback for errors, called from amqp thread, can be nullptr
   /// @param on_consume callback for consume on routing key name, called from amqp thread, null if no consume needed
   amqp( const std::string& address, std::string name, on_error_t on_err, on_consume_t on_consume = nullptr )
         : thread_pool_( "ampq", 1) // amqp is not thread safe, use only one thread
         , name_( std::move( name ) )
         , on_error_( std::move( on_err ) )
         , on_consume_( std::move( on_consume ) )
   {
      AMQP::Address amqp_address( address );
      ilog( "Connecting to AMQP address ${a} - Queue: ${q}...", ("a", std::string( amqp_address ))( "q", name_ ) );

      handler_ = std::make_unique<amqp_handler>( *this, thread_pool_.get_executor() );
      boost::asio::post( *handler_->amqp_strand(), [&]() {
         connection_ = std::make_unique<AMQP::TcpConnection>( handler_.get(), amqp_address );
      });
      wait();
   }

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

   /// Explicitly stop thread pool execution
   /// do not call from lambda's passed to publish or constructor e.g. on_error
   void stop() {
      if( handler_ ) {
         // drain amqp queue
         std::promise<void> stop_promise;
         boost::asio::post( *handler_->amqp_strand(), [&]() {
            stop_promise.set_value();
         } );
         stop_promise.get_future().wait();

         thread_pool_.stop();
         handler_.reset();
      }
   }

private:
   // called amqp thread
   void init( AMQP::TcpConnection* c ) {
      // declare channel
      channel_ = std::make_unique<AMQP::TcpChannel>( c );
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
      consumer.onReceived( [&](const AMQP::Message& message, uint64_t delivery_tag, bool redelivered) {
         bool r = on_consume_( message.body(), message.bodySize() );
         if( r ) {
            channel_->ack( delivery_tag );
         } else {
            channel_->reject( delivery_tag );
         }
      } );
   }

   // called from non-amqp thread
   void wait() {
      auto r = connected_.get_future().wait_for( std::chrono::seconds( 10 ) );
      if( r == std::future_status::timeout ) {
         on_error( "AMQP timeout declaring queue" );
      }
   }

   void on_error( const char* message ) {
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
   eosio::chain::named_thread_pool thread_pool_;
   std::unique_ptr<amqp_handler> handler_;
   std::unique_ptr<AMQP::TcpConnection> connection_;
   std::unique_ptr<AMQP::TcpChannel> channel_;
   std::promise<void> connected_;
   std::string name_;
   on_error_t on_error_;
   on_consume_t on_consume_;
};

} // namespace eosio
