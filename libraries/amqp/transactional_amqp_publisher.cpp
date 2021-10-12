#include <eosio/amqp/transactional_amqp_publisher.hpp>
#include <eosio/amqp/retrying_amqp_connection.hpp>
#include <eosio/amqp/util.hpp>

#include <fc/network/url.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger_config.hpp> //set_os_thread_name()

#include <amqpcpp.h>

#include <thread>
#include <chrono>
#include <future>
#include <algorithm>
#include <atomic>

#include <boost/asio.hpp>

namespace eosio {

struct transactional_amqp_publisher_impl {
   transactional_amqp_publisher_impl(const std::string& url, const std::string& exchange,
                                     const fc::microseconds& time_out,
                                     transactional_amqp_publisher::error_callback_t on_fatal_error);
   ~transactional_amqp_publisher_impl();
   void wait_for_signal(std::shared_ptr<boost::asio::signal_set> ss);
   void pump_queue();
   void publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue);
   void publish_message_direct(const std::string& routing_key, std::vector<char> data,
                               transactional_amqp_publisher::error_callback_t on_error);

   void channel_ready(AMQP::Channel* channel);
   void channel_failed();

   AMQP::Channel* channel = nullptr; ///< nullptr when channel is not up
   std::atomic_bool stopping = false;

   bool in_flight = false;
   std::deque<std::pair<std::string, std::vector<char>>> message_deque;

   std::thread thread;
   boost::asio::io_context ctx;

   single_channel_retrying_amqp_connection retrying_connection;
   transactional_amqp_publisher::error_callback_t on_fatal_error;

   const std::string exchange;
   const fc::microseconds ack_time_out;

   boost::asio::strand<boost::asio::io_context::executor_type> submitted_work_strand = boost::asio::make_strand(ctx);

   class acked_cond {
   private:
      std::mutex mtx_;
      bool acked_ = false;
      std::condition_variable ack_cond_;
      const std::chrono::microseconds time_out_us;
   public:
      explicit acked_cond(const fc::microseconds& time_out)
      : time_out_us(time_out.count()) {}
      void set() {
         {
            auto lock = std::scoped_lock(mtx_);
            acked_ = true;
         }
         ack_cond_.notify_one();
      }
      bool wait() {
         std::unique_lock<std::mutex> lk(mtx_);
         return ack_cond_.wait_for( lk, time_out_us, [&]{ return acked_; } );
      }
      void un_set() {
         auto lock = std::scoped_lock( mtx_ );
         acked_ = false;
      }
   };

   acked_cond ack_cond;
};

void transactional_amqp_publisher_impl::wait_for_signal(std::shared_ptr<boost::asio::signal_set> ss) {
   ss->async_wait([this, ss](const boost::system::error_code& ec, int) {
      if(ec)
         return;
      stopping = true;
      ack_cond.set();
      wait_for_signal(ss);
   });
}

transactional_amqp_publisher_impl::transactional_amqp_publisher_impl(const std::string& url, const std::string& exchange,
                                                                     const fc::microseconds& time_out,
                                                                     transactional_amqp_publisher::error_callback_t on_fatal_error)
  : retrying_connection(ctx, url, fc::microseconds(time_out.count()/2), [this](AMQP::Channel* c){channel_ready(c);}, [this](){channel_failed();})
  , on_fatal_error(std::move(on_fatal_error))
  , exchange(exchange)
  , ack_cond(time_out)
{
   thread = std::thread([this]() {
      fc::set_os_thread_name("tamqp");
      std::shared_ptr<boost::asio::signal_set> ss = std::make_shared<boost::asio::signal_set>(ctx, SIGINT, SIGTERM);
#ifdef SIGPIPE
      ss->add(SIGPIPE);
#endif
      wait_for_signal(ss);
      while(true) {
         try {
            ctx.run();
            break;
         }
         FC_LOG_AND_DROP();
      }
   });
}

transactional_amqp_publisher_impl::~transactional_amqp_publisher_impl() {
   stopping = true;

   // drain any remaining items on submitted queue
   std::promise<void> shutdown_promise;
   auto shutdown_future = shutdown_promise.get_future();
   boost::asio::post(submitted_work_strand, [&]() {
      shutdown_promise.set_value();
   });
   shutdown_future.wait();

   ctx.stop();
   thread.join();
}

void transactional_amqp_publisher_impl::channel_ready(AMQP::Channel* c) {
   channel = c;
   pump_queue();
}

void transactional_amqp_publisher_impl::channel_failed() {
   channel = nullptr;
}

void transactional_amqp_publisher_impl::pump_queue() {
   if(stopping || in_flight || !channel || message_deque.empty())
      return;

   channel->startTransaction();
   in_flight = true;
   for(const auto& msg : message_deque) {
      AMQP::Envelope envelope(msg.second.data(), msg.second.size());
      envelope.setPersistent();
      channel->publish(exchange, msg.first, envelope);
   }

   channel->commitTransaction().onSuccess([this](){
      message_deque.clear();
      ack_cond.set();
      in_flight = false;
   })
   .onFinalize([this]() {
      in_flight = false;
      //unfortuately we don't know if an error is due to something recoverable or if an error is due
      // to something unrecoverable. To know that, we need to pump the event queue some which may allow
      // channel_failed() to be called as needed. So always pump the event queue here
      ctx.post([this]() {
         pump_queue();
      } );
   });
}

void transactional_amqp_publisher_impl::publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue) {
   if( queue.empty() ) return;

   if(ctx.get_executor().running_in_this_thread()) {
      if( on_fatal_error ) on_fatal_error( "publish_message_raw called from AMQP thread" );
      return;
   }

   ack_cond.un_set();

   boost::asio::post( submitted_work_strand, [this, q = std::move( queue )]() mutable {
      if( !message_deque.empty() ) {
         if( on_fatal_error ) on_fatal_error( "message_deque not drained" );
         return;
      }
      message_deque = std::move( q );
      pump_queue();
   } );

   if( !ack_cond.wait() || stopping ) {
      if( on_fatal_error ) on_fatal_error( "time out waiting on AMQP commit transaction success ack" );
   }
}

void transactional_amqp_publisher_impl::publish_message_direct(const std::string& rk, std::vector<char> data,
                                                               transactional_amqp_publisher::error_callback_t on_error) {
   if(!ctx.get_executor().running_in_this_thread()) {
      boost::asio::post(submitted_work_strand,
                        [this, rk, d=std::move(data), on_e=std::move(on_error)]() mutable {
         publish_message_direct(rk, std::move(d), std::move(on_e));
      });
      return;
   }

   if(stopping || !channel) {
      std::string err = "AMQP connection " + fc::variant(retrying_connection.address()).as_string() +
            " to " + exchange + " not connected, dropping message " + rk;
      if(on_error) on_error(err);
      return;
   }

   AMQP::Envelope envelope(data.data(), data.size());
   envelope.setPersistent();
   channel->publish(exchange, rk, envelope);
}


transactional_amqp_publisher::transactional_amqp_publisher(const std::string& url, const std::string& exchange,
                                                           const fc::microseconds& time_out,
                                                           transactional_amqp_publisher::error_callback_t on_fatal_error) :
   my(new transactional_amqp_publisher_impl(url, exchange, time_out, std::move(on_fatal_error))) {}

void transactional_amqp_publisher::publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue) {
   my->publish_messages_raw( std::move( queue ) );
}

void transactional_amqp_publisher::publish_message_direct(const std::string& routing_key, std::vector<char> data, error_callback_t on_error) {
   my->publish_message_direct( routing_key, std::move(data), std::move(on_error) );
}

transactional_amqp_publisher::~transactional_amqp_publisher() = default;

}
