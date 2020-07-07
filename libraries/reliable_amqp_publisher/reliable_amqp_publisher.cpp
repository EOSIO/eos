#include <eosio/retrying_amqp_connection/retrying_amqp_connection.hpp>
#include <eosio/reliable_amqp_publisher/reliable_amqp_publisher.hpp>

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

   void channel_ready(AMQP::Channel* channel);
   void channel_failed();

   AMQP::Channel* channel = nullptr;  //nullptr when channel is not up
   std::atomic_bool stopping = false;

   unsigned in_flight = 0;
   std::deque<std::vector<char>> message_deque;

   std::thread thread;
   boost::asio::io_context ctx;

   single_channel_retrying_amqp_connection<boost::asio::io_context> retrying_connection;

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
   boost::asio::post(user_submitted_work_strand, [&]() {
      shutdown_promise.set_value();
   });
   shutdown_promise.get_future().wait();

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

   constexpr size_t max_msg_single_transaction = 16;
   const unsigned msgs_this_transaction = std::min(message_deque.size(), max_msg_single_transaction);

   channel->startTransaction();
   std::for_each(message_deque.begin(), message_deque.begin()+msgs_this_transaction, [this](const std::vector<char>& msg) {
      AMQP::Envelope envelope(msg.data(), msg.size());
      envelope.setPersistent();
      if(message_id)
         envelope.setMessageID(*message_id);
      channel->publish(exchange, routing_key, envelope);
   });

   in_flight = msgs_this_transaction;

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
      });
   });
}

reliable_amqp_publisher::reliable_amqp_publisher(const std::string& url, const std::string& exchange, const std::string& routing_key, const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id) :
   my(new reliable_amqp_publisher_impl(url, exchange, routing_key, unconfirmed_path, message_id)) {}

void reliable_amqp_publisher::publish_message_raw(std::vector<char>&& data) {
   if(!my->ctx.get_executor().running_in_this_thread()) {
      boost::asio::post(my->user_submitted_work_strand, [this, d=std::move(data)]() mutable {
         publish_message_raw(std::move(d));
      });
      return;
   }

   constexpr unsigned max_queued_messages = 1u<<20u;

   if(my->message_deque.size() > max_queued_messages) {
      if(my->logged_exceeded_max_depth == false)
         elog("AMQP connection ${a} publishing to \"${e}\" has reached ${max} unconfirmed messages; further messages will be dropped",
              ("a", my->retrying_connection.address())("e", my->exchange)("max", max_queued_messages));
      my->logged_exceeded_max_depth = true;
      return;
   }
   my->message_deque.emplace_back(std::move(data));
   my->pump_queue();
}

void reliable_amqp_publisher::post_on_io_context(std::function<void()>&& f) {
   boost::asio::post(my->user_submitted_work_strand, f);
}

reliable_amqp_publisher::~reliable_amqp_publisher() = default;

}