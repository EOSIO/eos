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

#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem.hpp>

namespace eosio {

struct reliable_amqp_publisher_callbacks {
   virtual void amqp_ready() = 0;
   virtual void amqp_error(const char* msg) = 0;
};

struct reliable_amqp_publisher_handler : AMQP::LibBoostAsioHandler {
   reliable_amqp_publisher_handler(reliable_amqp_publisher_callbacks* callbacks, boost::asio::io_context& ctx) :
      AMQP::LibBoostAsioHandler(ctx),
      cbs(callbacks) {}

   void onReady(AMQP::TcpConnection* c) override {
      cbs->amqp_ready();
   }

   //disable heartbeat for now
   uint16_t onNegotiate(AMQP::TcpConnection* connection, uint16_t interval) override {
      return 0;
   }

   void onError(AMQP::TcpConnection*, const char* message) override {
      cbs->amqp_error(message);
   }

   reliable_amqp_publisher_callbacks* cbs;
};

struct reliable_amqp_publisher_impl final : reliable_amqp_publisher_callbacks {
   reliable_amqp_publisher_impl(const std::string& url, const std::string& exchange, const boost::filesystem::path& unconfirmed_path) :
     data_file_path(unconfirmed_path), exchange(exchange) {
      std::string host = "localhost", user = "guest", pass = "guest", path;
      uint16_t port = 5672;
      fc::url parsed_url(url);
      FC_ASSERT(parsed_url.proto() == "amqp", "Only amqp:// URIs are supported for AMQP addresses");
      if(parsed_url.host() && parsed_url.host()->size())
         host = *parsed_url.host();
      if(parsed_url.user())
         user = *parsed_url.user();
      if(parsed_url.pass())
         pass = *parsed_url.pass();
      if(parsed_url.port())
         port = *parsed_url.port();
      if(parsed_url.path())
         path = parsed_url.path()->string();

      std::stringstream hostss;
      hostss << "amqp://" << user << ":" << pass << "@" << host << ":" << port << "/" << path;
      amqp_address = std::make_unique<AMQP::Address>(hostss.str());

      if(boost::filesystem::exists(data_file_path)) {
         try {
            fc::datastream<fc::cfile> file;
            file.set_file_path(data_file_path);
            file.open("rb");
            fc::raw::unpack(file, message_deque);
         } FC_RETHROW_EXCEPTIONS(error, "Failed to load previously cached AMQP messages from ${f}", ("f", (fc::path)data_file_path));
      }
      boost::system::error_code ec;
      boost::filesystem::remove(data_file_path, ec);

      thread = std::thread([this]() {
         try {
            fc::set_os_thread_name("amqp");
            retry_connection();
            ctx.run();
         }
         catch(...) {
            elog("Exception escaped from AMQP connection ${s} publishing to \"${e}\", this connection is permanently disabled now",
                 ("s", (std::string)*amqp_address)("e", this->exchange));
         }
      });
   }

   ~reliable_amqp_publisher_impl() {
      stopping = true;

      //drain any remaining items on queue
      std::promise<void> shutdown_promise;
      ctx.post([&]() {
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
            boost::filesystem::create_directories(data_file_path.parent_path());

            fc::datastream<fc::cfile> file;
            file.set_file_path(data_file_path);
            file.open("wb");
            fc::raw::pack(file, message_deque);
         } FC_LOG_AND_DROP();
      }
   }

   void retry_connection() {
      connected = false;
      channel.reset();
      connection.reset();
      handler.reset();
      next_retry_time = std::min(next_retry_time, 30u);
      retry_timer.expires_from_now(boost::posix_time::seconds(next_retry_time));
      retry_timer.async_wait([&](auto ec) {
         if(ec)
            return;
         handler = std::make_unique<reliable_amqp_publisher_handler>(this, ctx);
         try {
            connection = std::make_unique<AMQP::TcpConnection>(handler.get(), *amqp_address);
         } catch(...) { //this should never happen, but it could technically have thrown
            retry_connection();
         }
      });
      next_retry_time *= 2;
   }

   void amqp_ready() override {
      ilog("AMQP connection ${s} publishing to \"${e}\" established", ("s", (std::string)*amqp_address)("e", exchange));
      next_retry_time = 1;
      connected = true;
      channel = std::make_unique<AMQP::TcpChannel>(connection.get());
      channel->onError([this](const char* s) {
         elog("Channel error for AMQP connection ${s} publishing to \"${e}\": ${m}; restarting connection", ("s", (std::string)*amqp_address)("e", exchange)("m",s));
         retry_connection();
      });
   }

   void amqp_error(const char* message) override {
      wlog("AMQP connection ${a} publishing to \"${e}\" failed: ${m}; retrying", ("a", (std::string)*amqp_address)("m", message)("e", exchange));
      retry_connection();
   }

   void pump_queue() {
      if(stopping || in_flight || !connected || message_deque.empty())
         return;

      constexpr size_t max_msg_single_transaction = 16;
      const unsigned msgs_this_transaction = std::min(message_deque.size(), max_msg_single_transaction);

      channel->startTransaction();
      std::for_each(message_deque.begin(), message_deque.begin()+msgs_this_transaction, [this](const std::vector<char>& msg) {
         channel->publish(exchange, "", msg.data(), msg.size());
      });

      in_flight = msgs_this_transaction;

      channel->commitTransaction().onSuccess([this](){
         message_deque.erase(message_deque.begin(), message_deque.begin()+in_flight);
         logged_exceeded_max_depth = false;
      })
      .onFinalize([this]() {
         in_flight = 0;
         //unfortuately we don't know if an error is due to something recoverable or if an error is due
         // to something unrecoverable. To know that, we need to pump the event queue some so that Channel's or
         // Connection's onError is delivered. Failure to pump the event queue here can result in runaway recursion.
         ctx.post([this]() {
            pump_queue();
         });
      });
   }

   std::unique_ptr<AMQP::Address> amqp_address;
   std::unique_ptr<reliable_amqp_publisher_handler> handler;
   std::unique_ptr<AMQP::TcpConnection> connection;
   std::unique_ptr<AMQP::TcpChannel> channel;

   bool connected = false;
   bool stopping = false;

   unsigned in_flight = 0;
   std::deque<std::vector<char>> message_deque;

   std::thread thread;
   boost::asio::io_context ctx;
   boost::asio::deadline_timer retry_timer{ctx};
   unsigned next_retry_time = 1;

   const boost::filesystem::path data_file_path;

   const std::string exchange;
   bool logged_exceeded_max_depth = false;
};

reliable_amqp_publisher::reliable_amqp_publisher(const std::string& url, const std::string& exchange, const boost::filesystem::path& unconfirmed_path) :
   my(new reliable_amqp_publisher_impl(url, exchange, unconfirmed_path)) {}

void reliable_amqp_publisher::publish_message(std::vector<char>&& data) {
   my->ctx.dispatch([this, d=std::move(data)]() mutable {
      constexpr unsigned max_queued_messages = 1u<<20u;
      if(my->message_deque.size() > max_queued_messages) {
         if(my->logged_exceeded_max_depth == false)
            elog("AMQP connection ${a} publishing to \"${e}\" has reached ${max} unconfirmed messages; further messages will be dropped",
                 ("a", (std::string)*my->amqp_address)("e", my->exchange)("max", max_queued_messages));
         my->logged_exceeded_max_depth = true;
         return;
      }
      my->message_deque.emplace_back(std::move(d));
      my->pump_queue();
   });
}

void reliable_amqp_publisher::post_on_io_context(std::function<void()>&& f) {
   my->ctx.post(f);
}

reliable_amqp_publisher::~reliable_amqp_publisher() = default;

}