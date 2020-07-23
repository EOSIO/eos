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
                                bool explicit_send,
                                const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id);
   ~reliable_amqp_publisher_impl();
   void pump_queue();
   void publish_message_raw(fc::unsigned_int num, std::vector<char>&& data);

   void channel_ready(AMQP::Channel* channel);
   void channel_failed();

   AMQP::Channel* channel = nullptr;  //nullptr when channel is not up
   std::atomic_bool stopping = false;

   unsigned in_flight = 0;
   struct amqp_message {
      fc::unsigned_int  num = 0; ///< unique numbers indicates amqp transaction set
      std::vector<char> data;
   };
   std::deque<amqp_message> message_deque;

   std::thread thread;
   boost::asio::io_context ctx;

   single_channel_retrying_amqp_connection retrying_connection;

   const boost::filesystem::path data_file_path;

   const std::string exchange;
   const std::string routing_key;
   const bool explicit_send;
   const std::optional<std::string> message_id;
   bool logged_exceeded_max_depth = false;

   boost::asio::strand<boost::asio::io_context::executor_type> user_submitted_work_strand = boost::asio::make_strand(ctx);
};

reliable_amqp_publisher_impl::reliable_amqp_publisher_impl(const std::string& url, const std::string& exchange, const std::string& routing_key,
                                                           bool explicit_send,
                                                           const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id) :
<<<<<<< HEAD
  retrying_connection(ctx, url, [this](AMQP::Channel* c){channel_ready(c);}, [this](){channel_failed();}),
  data_file_path(unconfirmed_path), exchange(exchange), routing_key(routing_key), message_id(message_id) {
=======
  data_file_path(unconfirmed_path), exchange(exchange), routing_key(routing_key), explicit_send(explicit_send), message_id(message_id) {
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
>>>>>>> 4e2709134... Add explicit send feature

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

<<<<<<< HEAD
void reliable_amqp_publisher_impl::channel_ready(AMQP::Channel* c) {
   channel = c;
   pump_queue();
=======
void reliable_amqp_publisher_impl::retry_connection() {
   connected = false;
   channel.reset();
   connection.reset();
   handler.reset();
   const unsigned max_retry_seconds = 30u;
   next_retry_time = std::min(next_retry_time, max_retry_seconds);
   retry_timer.expires_from_now(boost::posix_time::seconds(next_retry_time));
   retry_timer.async_wait([&](auto ec) {
      if(ec)
         return;
      bringup_connection();
   });
   next_retry_time *= 2;
}

void reliable_amqp_publisher_impl::bringup_connection() {
   handler = std::make_unique<reliable_amqp_publisher_handler>(*this);
   try {
      connection = std::make_unique<AMQP::TcpConnection>(handler.get(), *amqp_address);
   } catch(...) { //this should never happen, but it could technically have thrown
      retry_connection();
   }
}

void reliable_amqp_publisher_impl::amqp_ready() {
   ilog("AMQP connection ${s} publishing to \"${e}\" established", ("s", (std::string)*amqp_address)("e", exchange));
   next_retry_time = 1;
   connected = true;
   channel = std::make_unique<AMQP::TcpChannel>(connection.get());
   channel->onError([this](const char* s) {
      elog("Channel error for AMQP connection ${s} publishing to \"${e}\": ${m}; restarting connection", ("s", (std::string)*amqp_address)("e", exchange)("m",s));
      retry_connection();
   });
   if( !explicit_send )
      pump_queue();
>>>>>>> 4e2709134... Add explicit send feature
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
      if( in_flight > max_msg_single_transaction && msg.num == fc::unsigned_int(0) )
         break;

      AMQP::Envelope envelope(msg.data.data(), msg.data.size());
      envelope.setPersistent();
      if(message_id)
         envelope.setMessageID(*message_id);
      channel->publish(exchange, routing_key, envelope);

      prev = msg.num;
   }

   channel->commitTransaction().onSuccess([this](){
      message_deque.erase(message_deque.begin(), message_deque.begin()+in_flight);
      logged_exceeded_max_depth = false;
   })
   .onFinalize([this]() {
      in_flight = 0;
      //unfortuately we don't know if an error is due to something recoverable or if an error is due
<<<<<<< HEAD
      // to something unrecoverable. To know that, we need to pump the event queue some which may allow
      // channel_failed() to be called as needed. So always pump the event queue here
      ctx.post([this]() {
         pump_queue();
      });
=======
      // to something unrecoverable. To know that, we need to pump the event queue some so that Channel's or
      // Connection's onError is delivered. Failure to pump the event queue here can result in runaway recursion.
      if( !explicit_send ) {
         handler->amqp_strand()->post( [this]() {
            pump_queue();
         } );
      }
>>>>>>> 4e2709134... Add explicit send feature
   });
}

void reliable_amqp_publisher_impl::publish_message_raw(fc::unsigned_int num, std::vector<char>&& data) {
   if(!ctx.get_executor().running_in_this_thread()) {
      boost::asio::post(user_submitted_work_strand, [this, num, d=std::move(data)]() mutable {
         publish_message_raw(num, std::move(d));
      });
      return;
   }

   constexpr unsigned max_queued_messages = 1u<<20u;

   if(message_deque.size() > max_queued_messages) {
      if(logged_exceeded_max_depth == false)
         elog("AMQP connection ${a} publishing to \"${e}\" has reached ${max} unconfirmed messages; further messages will be dropped",
<<<<<<< HEAD
              ("a", my->retrying_connection.address())("e", my->exchange)("max", max_queued_messages));
      my->logged_exceeded_max_depth = true;
=======
              ("a", (std::string)*amqp_address)("e", exchange)("max", max_queued_messages));
      logged_exceeded_max_depth = true;
>>>>>>> 4e2709134... Add explicit send feature
      return;
   }
   message_deque.emplace_back(amqp_message{num, std::move(data)});
   if( !explicit_send )
      pump_queue();
}

reliable_amqp_publisher::reliable_amqp_publisher(const std::string& url, const std::string& exchange, const std::string& routing_key,
                                                 bool explicit_send,
                                                 const boost::filesystem::path& unconfirmed_path, const std::optional<std::string>& message_id) :
   my(new reliable_amqp_publisher_impl(url, exchange, routing_key, explicit_send, unconfirmed_path, message_id)) {}

void reliable_amqp_publisher::publish_message_raw(std::vector<char>&& data) {
   my->publish_message_raw( 0, std::move( data ) );
}

void reliable_amqp_publisher::publish_message_raw(fc::unsigned_int num, std::vector<char>&& data) {
   my->publish_message_raw( num, std::move( data ) );
}

void reliable_amqp_publisher::send_messages() {
   boost::asio::post(my->user_submitted_work_strand, [&my=my]() {
      my->pump_queue();
   } );
}

void reliable_amqp_publisher::post_on_io_context(std::function<void()>&& f) {
   boost::asio::post(my->user_submitted_work_strand, f);
}

reliable_amqp_publisher::~reliable_amqp_publisher() = default;

}

FC_REFLECT( eosio::reliable_amqp_publisher_impl::amqp_message, (num)(data) )
