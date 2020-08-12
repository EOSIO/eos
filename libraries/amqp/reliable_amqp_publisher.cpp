#include <eosio/amqp/retrying_amqp_connection.hpp>
#include <eosio/amqp/reliable_amqp_publisher.hpp>
#include <eosio/amqp/util.hpp>

#include <fc/network/url.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/cfile.hpp>
#include <fc/log/logger_config.hpp> //set_os_thread_name()

#include <amqpcpp.h>
#include <amqpcpp/libboostasio.h>

#include <thread>
#include <chrono>
#include <future>
#include <algorithm>
#include <atomic>

#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>

namespace eosio {

struct reliable_amqp_publisher_handler;

struct reliable_amqp_publisher_impl {
   reliable_amqp_publisher_impl(const std::string& url, const std::string& exchange, const std::string& routing_key,
                                const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id);
   ~reliable_amqp_publisher_impl();
   void pump_queue();
   void publish_message_raw(std::vector<char>&& data);
   void publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue);
   void publish_message_direct(const std::string& routing_key, std::vector<char> data);
   bool verify_max_queue_size();

   void channel_ready(AMQP::Channel* channel);
   void channel_failed();

   AMQP::Channel* channel = nullptr;  //nullptr when channel is not up
   std::atomic_bool stopping = false;

   unsigned in_flight = 0;
   fc::unsigned_int batch_num = 0;
   struct amqp_message {
      fc::unsigned_int  num = 0; ///< unique numbers indicates amqp transaction set (reset on clean restart)
      std::string       routing_key;
      std::vector<char> data;
   };
   std::deque<amqp_message> message_deque;

   std::thread thread;
   boost::asio::io_context ctx;

   single_channel_retrying_amqp_connection retrying_connection;

   const boost::filesystem::path data_file_path;

   const std::string exchange;
   const std::string routing_key;
   const std::optional<std::string> message_id;
   bool logged_exceeded_max_depth = false;

   boost::asio::strand<boost::asio::io_context::executor_type> user_submitted_work_strand = boost::asio::make_strand(ctx);
};

reliable_amqp_publisher_impl::reliable_amqp_publisher_impl(const std::string& url, const std::string& exchange, const std::string& routing_key,
                                                           const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id) :
  retrying_connection(ctx, url, [this](AMQP::Channel* c){channel_ready(c);}, [this](){channel_failed();}),
  data_file_path(unconfirmed_path), exchange(exchange), routing_key(routing_key), message_id(message_id) {

   boost::system::error_code ec;
   boost::filesystem::create_directories(data_file_path.parent_path(), ec);

   if(boost::filesystem::exists(data_file_path)) {
      try {
         fc::datastream<fc::cfile> file;
         file.set_file_path(data_file_path);
         file.open("rb");
         fc::raw::unpack(file, message_deque);
         if( !message_deque.empty() )
            batch_num = message_deque.back().num;
      } FC_RETHROW_EXCEPTIONS(error, "Failed to load previously unconfirmed AMQP messages from ${f}", ("f", (fc::path)data_file_path));
   }
   else {
      boost::filesystem::ofstream o(data_file_path);
      FC_ASSERT(o.good(), "Failed to create unconfirmed AMQP message file at ${f}", ("f", (fc::path)data_file_path));
   }
   boost::filesystem::remove(data_file_path, ec);

   thread = std::thread([this]() {
      fc::set_os_thread_name("amqp");
      while(true) {
         try {
            ctx.run();
            break;
         }
         FC_LOG_AND_DROP();
      }
   });
}

reliable_amqp_publisher_impl::~reliable_amqp_publisher_impl() {
   stopping = true;

   //drain any remaining items on user submitted queue
   std::promise<void> shutdown_promise;
   auto shutdown_future = shutdown_promise.get_future();
   boost::asio::post(user_submitted_work_strand, [&]() {
      shutdown_promise.set_value();
   });
   shutdown_future.wait();

   ctx.stop();
   thread.join();

   //if a message is in flight, let it try to run to completion
   if(in_flight) {
      auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
      ctx.restart();
      while(in_flight && ctx.run_one_until(until));
   }

   if(message_deque.size()) {
      try {
         fc::datastream<fc::cfile> file;
         file.set_file_path(data_file_path);
         file.open("wb");
         fc::raw::pack(file, message_deque);
      } FC_LOG_AND_DROP();
   }
}

void reliable_amqp_publisher_impl::channel_ready(AMQP::Channel* c) {
   channel = c;
   pump_queue();
}

void reliable_amqp_publisher_impl::channel_failed() {
   channel = nullptr;
}

void reliable_amqp_publisher_impl::pump_queue() {
   if(stopping || in_flight || !channel || message_deque.empty())
      return;

   constexpr size_t max_msg_single_transaction = 16; // if msg.num == 0

   channel->startTransaction();
   in_flight = 0;
   fc::unsigned_int prev = 0;
   for( auto i = message_deque.begin(), end = message_deque.end(); i != end; ++i, ++in_flight) {
      const amqp_message& msg = *i;
      if( in_flight != 0 && msg.num != prev )
         break;
      if( in_flight > max_msg_single_transaction && msg.num == 0u )
         break;

      AMQP::Envelope envelope(msg.data.data(), msg.data.size());
      envelope.setPersistent();
      if(message_id)
         envelope.setMessageID(*message_id);
      channel->publish(exchange, msg.routing_key.empty() ? routing_key : msg.routing_key, envelope);

      prev = msg.num;
   }

   channel->commitTransaction().onSuccess([this](){
      message_deque.erase(message_deque.begin(), message_deque.begin()+in_flight);
      logged_exceeded_max_depth = false;
   })
   .onFinalize([this]() {
      in_flight = 0;
      //unfortuately we don't know if an error is due to something recoverable or if an error is due
      // to something unrecoverable. To know that, we need to pump the event queue some which may allow
      // channel_failed() to be called as needed. So always pump the event queue here
      ctx.post([this]() {
         pump_queue();
      } );
   });
}

bool reliable_amqp_publisher_impl::verify_max_queue_size() {
   constexpr unsigned max_queued_messages = 1u << 20u;

   if(message_deque.size() > max_queued_messages) {
      if(logged_exceeded_max_depth == false)
         elog("AMQP connection ${a} publishing to \"${e}\" has reached ${max} unconfirmed messages; dropping messages",
              ("a", retrying_connection.address())("e", exchange)("max", max_queued_messages));
      logged_exceeded_max_depth = true;
      return false;
   }
   return true;
}

void reliable_amqp_publisher_impl::publish_message_raw(std::vector<char>&& data) {
   if(!ctx.get_executor().running_in_this_thread()) {
      boost::asio::post(user_submitted_work_strand, [this, d=std::move(data)]() mutable {
         publish_message_raw(std::move(d));
      });
      return;
   }

   if( !verify_max_queue_size() ) return;

   message_deque.emplace_back(amqp_message{0, "", std::move(data)});
   pump_queue();
}

void reliable_amqp_publisher_impl::publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue) {
   if(!ctx.get_executor().running_in_this_thread()) {
      boost::asio::post(user_submitted_work_strand, [this, q=std::move(queue)]() mutable {
         publish_messages_raw(std::move(q));
      });
      return;
   }

   if( !verify_max_queue_size() ) return;

   ++batch_num.value;
   if( batch_num == 0u ) ++batch_num.value;
   for( auto& d : queue ) {
      message_deque.emplace_back(amqp_message{batch_num, std::move(d.first), std::move(d.second)});
   }
   pump_queue();
}

void reliable_amqp_publisher_impl::publish_message_direct(const std::string& rk, std::vector<char> data) {
   if(!ctx.get_executor().running_in_this_thread()) {
      boost::asio::post(user_submitted_work_strand, [this, rk, d=std::move(data)]() mutable {
         publish_message_direct(rk, std::move(d));
      });
      return;
   }

   if(stopping || !channel) {
      elog( "AMQP connection ${a} to ${e} not connected dropping message ${rk}",
            ("a", retrying_connection.address())("e", exchange)("rk", rk));
      return;
   }

   AMQP::Envelope envelope(data.data(), data.size());
   envelope.setPersistent();
   if(message_id)
      envelope.setMessageID(*message_id);
   channel->publish(exchange, rk.empty() ? routing_key : rk, envelope);
}


reliable_amqp_publisher::reliable_amqp_publisher(const std::string& url, const std::string& exchange, const std::string& routing_key,
                                                 const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id) :
   my(new reliable_amqp_publisher_impl(url, exchange, routing_key, unconfirmed_path, message_id)) {}

void reliable_amqp_publisher::publish_message_raw(std::vector<char>&& data) {
   my->publish_message_raw( std::move( data ) );
}

void reliable_amqp_publisher::publish_messages_raw(std::deque<std::pair<std::string, std::vector<char>>>&& queue) {
   my->publish_messages_raw( std::move( queue ) );
}

void reliable_amqp_publisher::publish_message_direct(const std::string& routing_key, std::vector<char> data) {
   my->publish_message_direct( routing_key, std::move(data) );
}

void reliable_amqp_publisher::post_on_io_context(std::function<void()>&& f) {
   boost::asio::post(my->user_submitted_work_strand, f);
}

reliable_amqp_publisher::~reliable_amqp_publisher() = default;

}

FC_REFLECT( eosio::reliable_amqp_publisher_impl::amqp_message, (num)(routing_key)(data) )
