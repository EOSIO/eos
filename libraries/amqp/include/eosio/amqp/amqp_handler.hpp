#pragma once

#include <eosio/amqp/retrying_amqp_connection.hpp>
#include <eosio/amqp/util.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>
#include <amqpcpp.h>
#include <condition_variable>


namespace eosio {

/// Designed to work with internal io_service running on a dedicated thread.
/// Calls on_consume from internal thread.
/// ack() & reject() & publish methods can be called from any thread, but should be sync'ed with stop() & destructor.
/// Constructor, stop(), destructor should be called from same thread.
class amqp_handler {
public:
   // called from amqp thread or calling thread, but not concurrently
   using on_error_t = std::function<void(const std::string& err)>;
   // delivery_tag type of consume, use for ack/reject
   using delivery_tag_t = uint64_t;
   // called from amqp thread on consume of message
   using on_consume_t = std::function<void(const AMQP::Message& message, delivery_tag_t delivery_tag, bool redelivered)>;

   /// @param address AMQP address
   /// @param name AMQP routing key
   /// @param on_err callback for errors, called from amqp thread or caller thread, can be nullptr
   /// @param on_consume callback for consume on routing key name, called from amqp thread, null if no consume needed.
   ///        user required to ack/reject delivery_tag for each callback.
   amqp_handler( const std::string& address, std::string name, on_error_t on_err, on_consume_t on_consume = nullptr )
         : amqp_handler( address, std::move(name), "", "", std::move(on_err), std::move(on_consume))
   {}

   /// @param address AMQP address
   /// @param exchange_name AMQP exchange to send message to
   /// @param exchange_type AMQP exhcnage type
   /// @param on_err callback for errors, called from amqp thread or caller thread, can be nullptr
   /// @param on_consume callback for consume on routing key name, called from amqp thread, null if no consume needed.
   ///        user required to ack/reject delivery_tag for each callback.
   amqp_handler( const std::string& address, std::string exchange_name, std::string exchange_type, on_error_t on_err, on_consume_t on_consume = nullptr )
         : amqp_handler( address, "", std::move(exchange_name), std::move(exchange_type), std::move(on_err), std::move(on_consume))
   {}

   /// drain queue and stop thread
   ~amqp_handler() {
      stop();
   }

   /// publish to AMQP address with routing key name
   //  on_error() called if not connected
   void publish( std::string exchange, std::string correlation_id, std::vector<char> buf ) {
      boost::asio::post( thread_pool_.get_executor(),
            [my=this, exchange=std::move(exchange), cid=std::move(correlation_id), buf=std::move(buf)]() {
               AMQP::Envelope env( buf.data(), buf.size() );
               env.setCorrelationID( cid );
               if( my->channel_ )
                  my->channel_->publish( exchange, my->name_, env, 0 );
               else
                  my->on_error( "AMQP Unable to publish: " + cid );
      } );
   }

   /// publish to AMQP calling f() -> std::vector<char> on amqp thread
   //  on_error() called if not connected
   template<typename Func>
   void publish( std::string exchange, std::string correlation_id, Func f ) {
      boost::asio::post( thread_pool_.get_executor(),
            [my=this, exchange=std::move(exchange), cid=std::move(correlation_id), f=std::move(f)]() {
               std::vector<char> buf = f();
               AMQP::Envelope env( buf.data(), buf.size() );
               env.setCorrelationID( cid );
               if( my->channel_ )
                  my->channel_->publish( exchange, my->name_, env, 0 );
               else
                  my->on_error( "AMQP Unable to publish: " + cid );
      } );
   }

   /// ack consume message
   /// @param multiple true if given delivery_tag and all previous should be ack'ed
   void ack( const delivery_tag_t& delivery_tag, bool multiple = false ) {
      boost::asio::post( thread_pool_.get_executor(),
            [my = this, delivery_tag, multiple]() {
               try {
                  if( my->channel_ )
                     my->channel_->ack( delivery_tag, multiple ? AMQP::multiple : 0 );
                  else
                     my->on_error( "AMQP Unable to ack: " + std::to_string(delivery_tag) );
               } FC_LOG_AND_DROP()
            } );
   }

   // reject consume message
   void reject( const delivery_tag_t& delivery_tag ) {
      boost::asio::post( thread_pool_.get_executor(),
            [my = this, delivery_tag]() {
               try {
                  if( my->channel_ )
                     my->channel_->reject( delivery_tag );
                  else
                     my->on_error( "AMQP Unable to reject: " + std::to_string(delivery_tag) );
               } FC_LOG_AND_DROP()
            } );
   }

   /// Explicitly stop thread pool execution
   /// Not thread safe, call only once from constructor/destructor thread
   /// Do not call from lambda's passed to publish or constructor e.g. on_error
   void stop() {
      if( first_connect_.un_set() ) {
         // drain amqp queue
         std::promise<void> stop_promise;
         auto future = stop_promise.get_future();
         boost::asio::post( thread_pool_.get_executor(), [&]() {
            if( channel_ && channel_->usable() ) {
               auto& cb = channel_->close();
               cb.onFinalize( [&]() {
                  try {
                     stop_promise.set_value();
                  } catch(...) {}
               } );
               cb.onError( [&](const char* message) {
                  elog( "Error on stop: ${m}", ("m", message) );
                  try {
                     stop_promise.set_value();
                  } catch(...) {}
               } );
            } else {
               stop_promise.set_value();
            }
         } );
         future.wait();

         thread_pool_.stop();
      }
   }

private:

   amqp_handler(const std::string& address, std::string name, std::string exchange_name, std::string exchange_type,
                on_error_t on_err, on_consume_t on_consume)
         : first_connect_()
         , thread_pool_( "ampq", 1 ) // amqp is not thread safe, use only one thread
         , amqp_connection_(  thread_pool_.get_executor(), address, [this](AMQP::Channel* c){channel_ready(c);}, [this](){channel_failed();} )
         , name_( std::move( name ) )
         , exchange_name_( std::move( exchange_name ) )
         , exchange_type_( std::move( exchange_type ) )
         , on_error_( std::move( on_err ) )
         , on_consume_( std::move( on_consume ) )
   {
      AMQP::Address amqp_address( address );
      ilog( "Connecting to AMQP address ${a} - Queue: ${q}...", ("a", amqp_address)("q", name_) );

      wait();
   }

   // called from non-amqp thread
   void wait() {
      if( !first_connect_.wait() ) {
         boost::asio::post( thread_pool_.get_executor(), [this](){
            on_error( "AMQP timeout connecting and declaring queue" );
         } );
      }
   }

   // called from amqp thread
   void channel_ready(AMQP::Channel* c) {
      channel_ = c;
      init();
   }

   // called from amqp thread
   void channel_failed() {
      channel_ = nullptr;
   }

   // called from amqp thread
   void init() {
      if(!channel_) return;
      if( !exchange_type_.empty() ) {
         auto type = AMQP::direct;
         if (exchange_type_ == "fanout") {
            type = AMQP::fanout;
         } else if (exchange_type_ != "direct") {
            on_error( "Unsupported exchange type: " + exchange_type_ );
            first_connect_.set();
            return;
         }

         auto& exchange = channel_->declareExchange( exchange_name_, type, AMQP::durable);
         exchange.onSuccess( [this]() {
            dlog( "AMQP declare exchange Successfully!\n Exchange ${e}", ("e", exchange_name_) );
            init_consume();
         } );
         exchange.onError([this](const char* error_message) {
            on_error( std::string("AMQP Queue error: ") + error_message );
            first_connect_.set();
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
            first_connect_.set();
         } );
      }
   }

   // called from amqp thread
   void init_consume() {
      if( !on_consume_ ) {
         first_connect_.set();
         return;
      }

      auto& consumer = channel_->consume( name_ );
      consumer.onSuccess( [&]( const std::string& consumer_tag ) {
         dlog( "consume started: ${tag}", ("tag", consumer_tag) );
         first_connect_.set();
      } );
      consumer.onError( [&]( const char* message ) {
         wlog( "consume failed: ${e}", ("e", message) );
         on_error( message );
         first_connect_.set();
      } );
      static_assert( std::is_same_v<on_consume_t, AMQP::MessageCallback>, "AMQP::MessageCallback interface changed" );
      consumer.onReceived( on_consume_ );
   }

   // called from amqp thread
   void on_error( const std::string& message ) {
      if( on_error_ ) on_error_( message );
   }

private:
   class start_cond {
   private:
      std::mutex mtx_;
      bool started_ = false;
      std::condition_variable start_cond_;
   public:
      void set() {
         {
            auto lock = std::scoped_lock(mtx_);
            started_ = true;
         }
         start_cond_.notify_one();
      }
      bool wait() {
         std::unique_lock<std::mutex> lk(mtx_);
         return start_cond_.wait_for( lk, std::chrono::seconds( 10 ), [&]{ return started_; } );
      }
      // only unset on shutdown, return prev value
      bool un_set() {
         auto lock = std::scoped_lock( mtx_ );
         bool s = started_;
         started_ = false;
         return s;
      }
   };

private:
   start_cond first_connect_;
   eosio::chain::named_thread_pool thread_pool_;
   single_channel_retrying_amqp_connection amqp_connection_;
   AMQP::Channel* channel_ = nullptr; // null when not connected
   std::string name_;
   std::string exchange_name_;
   std::string exchange_type_;
   on_error_t on_error_;
   on_consume_t on_consume_;
};

} // namespace eosio
