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

   /// Blocks until connection successful or error
   /// @param address AMQP address
   /// @param on_err callback for errors, called from amqp thread or caller thread, can be nullptr
   amqp_handler( const std::string& address, on_error_t on_err )
         : first_connect_()
         , thread_pool_( "ampq", 1 ) // amqp is not thread safe, use only one thread
         , amqp_connection_(  thread_pool_.get_executor(), address,
                              [this](AMQP::Channel* c){channel_ready(c);}, [this](){channel_failed();} )
         , on_error_( std::move( on_err ) )
   {
      ilog( "Connecting to AMQP address ${a} ...", ("a", amqp_connection_.address()) );

      wait();
   }

   /// Declare a durable exchange, blocks until successful or error
   /// @param exchange_name AMQP exchange to declare
   /// @param exchange_type AMQP exchange type, empty (direct), "fanout", or "direct" anything else calls on_err
   void declare_exchange(std::string exchange_name, const std::string& exchange_type) {
      auto type = AMQP::direct;
      if( !exchange_type.empty() ) {
         if (exchange_type == "fanout") {
            type = AMQP::fanout;
         } else if (exchange_type != "direct") {
            on_error( "AMQP unsupported exchange type: " + exchange_type );
            return;
         }
      }

      start_cond cond;
      boost::asio::post( thread_pool_.get_executor(),[this, &cond, en=exchange_name, type]() {
         try {
            if( !channel_ ) {
               elog( "AMQP not connected to channel ${a}", ("a", amqp_connection_.address()) );
               on_error( "AMQP not connected to channel" );
               return;
            }

            auto& exchange = channel_->declareExchange( en, type, AMQP::durable);
            exchange.onSuccess( [this, &cond, en]() {
               dlog( "AMQP declare exchange successful, exchange ${e}, for ${a}",
                     ("e", en)("a", amqp_connection_.address()) );
               cond.set();
            } );
            exchange.onError([this, &cond, en](const char* error_message) {
               elog( "AMQP unable to declare exchange ${e}, for ${a}", ("e", en)("a", amqp_connection_.address()) );
               on_error( std::string("AMQP Queue error: ") + error_message );
               cond.set();
            });
            return;
         } FC_LOG_AND_DROP()
         cond.set();
      } );

      if( !cond.wait() ) {
         elog( "AMQP timeout declaring exchange: ${q} for ${a}", ("q", exchange_name)("a", amqp_connection_.address()) );
         on_error( "AMQP timeout declaring exchange: " + exchange_name );
      }
   }

   /// Declare a durable queue, blocks until successful or error
   /// @param queue_name AMQP queue name to declare
   void declare_queue(std::string queue_name) {
      start_cond cond;
      boost::asio::post( thread_pool_.get_executor(), [this, &cond, qn=queue_name]() mutable {
          try {
             if( !channel_ ) {
                elog( "AMQP not connected to channel ${a}", ("a", amqp_connection_.address()) );
                on_error( "AMQP not connected to channel" );
                return;
             }

             auto& queue = channel_->declareQueue( qn, AMQP::durable );
             queue.onSuccess(
                   [this, &cond]( const std::string& name, uint32_t message_count, uint32_t consumer_count ) {
                      dlog( "AMQP queue ${q}, messages: ${mc}, consumers: ${cc}, for ${a}",
                            ("q", name)("mc", message_count)("cc", consumer_count)("a", amqp_connection_.address()) );
                      cond.set();
                   } );
             queue.onError( [this, &cond, qn]( const char* error_message ) {
                elog( "AMQP error declaring queue ${q} for ${a}", ("q", qn)("a", amqp_connection_.address()) );
                on_error( error_message );
                cond.set();
             } );
             return;
          } FC_LOG_AND_DROP()
          cond.set();
       } );

      if( !cond.wait() ) {
         elog( "AMQP timeout declaring queue: ${q} for ${a}", ("q", queue_name)("a", amqp_connection_.address()) );
         on_error( "AMQP timeout declaring queue: " + queue_name );
      }
   }

   /// drain queue and stop thread
   ~amqp_handler() {
      stop();
   }

   /// publish to AMQP, on_error() called if not connected
   void publish( std::string exchange, std::string queue_name, std::string correlation_id, std::string reply_to,
                 std::vector<char> buf ) {
      boost::asio::post( thread_pool_.get_executor(),
                         [my=this, exchange=std::move(exchange), qn=std::move(queue_name),
                          cid=std::move(correlation_id), rt=std::move(reply_to), buf=std::move(buf)]() mutable {
            try {
               if( !my->channel_ ) {
                  elog( "AMQP not connected to channel ${a}", ("a", my->amqp_connection_.address()) );
                  my->on_error( "AMQP not connected to channel" );
                  return;
               }

               AMQP::Envelope env( buf.data(), buf.size() );
               if(!cid.empty()) env.setCorrelationID( std::move( cid ) );
               if(!rt.empty()) env.setReplyTo( std::move( rt ) );
               my->channel_->publish( exchange, qn, env, 0 );
            } FC_LOG_AND_DROP()
      } );
   }

   /// publish to AMQP calling f() -> std::vector<char> on amqp thread
   //  on_error() called if not connected
   template<typename Func>
   void publish( std::string exchange, std::string queue_name, std::string correlation_id, std::string reply_to, Func f ) {
      boost::asio::post( thread_pool_.get_executor(),
                         [my=this, exchange=std::move(exchange), qn=std::move(queue_name),
                          cid=std::move(correlation_id), rt=std::move(reply_to), f=std::move(f)]() mutable {
            try {
               if( !my->channel_ ) {
                  elog( "AMQP not connected to channel ${a}", ("a", my->amqp_connection_.address()) );
                  my->on_error( "AMQP not connected to channel" );
                  return;
               }

               std::vector<char> buf = f();
               AMQP::Envelope env( buf.data(), buf.size() );
               if(!cid.empty()) env.setCorrelationID( std::move( cid ) );
               if(!rt.empty()) env.setReplyTo( std::move(rt) );
               my->channel_->publish( exchange, qn, env, 0 );
            } FC_LOG_AND_DROP()
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

   /// Start consuming from provided queue, does not block.
   /// Only expected to be called once as changing queue is not supported, will call on_err if called more than once
   /// @param queue_name name of queue to consume from, must pre-exist
   /// @param on_consume callback for consume on routing key name, called from amqp thread.
   ///        user required to ack/reject delivery_tag for each callback.
   /// @param recover if true recover all messages that were not yet acked
   //                 asks the server to redeliver all unacknowledged messages on the channel
   //                 zero or more messages may be redelivered
   void start_consume(std::string queue_name, on_consume_t on_consume, bool recover) {
      boost::asio::post( thread_pool_.get_executor(),
                         [this, qn{std::move(queue_name)}, on_consume{std::move(on_consume)}, recover]() mutable {
         try {
            if( on_consume_ ) {
               on_error("AMQP already consuming from: " + queue_name_ + ", unable to consume from: " + qn);
               return;
            } else if( !on_consume ) {
               on_error("AMQP nullptr passed for on_consume callback");
               return;
            }
            queue_name_ = std::move(qn);
            on_consume_ = std::move(on_consume);
            init_consume(recover);
         } FC_LOG_AND_DROP()
      } );
   }

private:

   // called from non-amqp thread
   void wait() {
      if( !first_connect_.wait() ) {
         elog( "AMQP timeout connecting to: ${a}", ("a", amqp_connection_.address()) );
         on_error( "AMQP timeout connecting" );
      }
   }

   // called from amqp thread
   void channel_ready(AMQP::Channel* c) {
      ilog( "AMQP Channel ready: ${id}, for ${a}", ("id", c ? c->id() : 0)("a", amqp_connection_.address()) );
      channel_ = c;
      init_consume(true);
      first_connect_.set();
   }

   // called from amqp thread
   void channel_failed() {
      elog( "AMQP connection failed to: ${a}", ("a", amqp_connection_.address()) );
      channel_ = nullptr;
   }

   // called from amqp thread
   void init_consume(bool recover) {
      if( channel_ && on_consume_ ) {
         if (recover) {
            channel_->recover(AMQP::requeue)
              .onSuccess( [&]() { dlog( "successfully started channel recovery" ); } )
              .onError( [&]( const char* message ) {
                 elog( "channel recovery failed ${e}", ("e", message) );
                 on_error( "AMQP channel recovery failed" );
              } );
         }

         auto &consumer = channel_->consume(queue_name_);
         consumer.onSuccess([&](const std::string &consumer_tag) {
            ilog("consume started: ${tag}, for ${a}", ("tag", consumer_tag)("a", amqp_connection_.address()));
         });
         consumer.onError([&](const char *message) {
            elog("consume failed: ${e}, for ${a}", ("e", message)("a", amqp_connection_.address()));
            on_error(message);
         });
         static_assert(std::is_same_v<on_consume_t, AMQP::MessageCallback>, "AMQP::MessageCallback interface changed");
         consumer.onReceived(on_consume_);
      }
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
   on_error_t on_error_;
   on_consume_t on_consume_;
   std::string queue_name_;
};

} // namespace eosio
