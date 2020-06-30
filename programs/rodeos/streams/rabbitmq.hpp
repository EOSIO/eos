#pragma once

#include "amqpcpp.h"
#include "amqpcpp/libboostasio.h"
#include "amqpcpp/linux_tcp.h"
#include "stream.hpp"
#include <eosio/reliable_amqp_publisher/reliable_amqp_publisher.hpp>
#include <fc/log/logger.hpp>
#include <boost/filesystem/operations.hpp>
#include <memory>

namespace b1 {

class rabbitmq_handler;

class rabbitmq : public stream_handler {
   std::shared_ptr<eosio::reliable_amqp_publisher> amqp_publisher_;
   AMQP::Address                        address_;
   std::shared_ptr<rabbitmq_handler>    handler_;
   std::shared_ptr<AMQP::TcpConnection> connection_;
   std::shared_ptr<AMQP::TcpChannel>    channel_;
   std::string                          exchange_name_;
   std::string                          queue_name_;
   std::vector<eosio::name>             routes_;
   boost::filesystem::path              unconfirmed_path_;
   // capture all messages per block and send as one amqp transaction
   std::deque<std::pair<std::string, std::vector<char>>> queue_;

 public:
   rabbitmq(boost::asio::io_service& io_service, std::vector<eosio::name> routes, const AMQP::Address& address,
            std::string queue_name, const boost::filesystem::path& unconfirmed_path)
       : address_(address), queue_name_( std::move( queue_name)), routes_( std::move( routes)), unconfirmed_path_( unconfirmed_path) {
      ilog("Connecting to RabbitMQ address ${a} - Queue: ${q}...", ("a", std::string(address))( "q", queue_name_));

      init(io_service, address);

      declare_queue();

   }

   rabbitmq(boost::asio::io_service& io_service, std::vector<eosio::name> routes, const AMQP::Address& address,
            std::string exchange_name, std::string exchange_type, const boost::filesystem::path& unconfirmed_path)
       : address_(address), exchange_name_( std::move( exchange_name)), routes_( std::move( routes)), unconfirmed_path_( unconfirmed_path) {
      ilog("Connecting to RabbitMQ address ${a} - Exchange: ${e}...", ("a", std::string(address))( "e", exchange_name_));

      init(io_service, address);

      declare_exchange(exchange_type);

      amqp_publisher_ = std::make_shared<eosio::reliable_amqp_publisher>(address, exchange_name, "", unconfirmed_path);
   }

   const std::vector<eosio::name>& get_routes() const override { return routes_; }

   void start_block(uint32_t block_num) override {
      queue_.clear();
   }

   void stop_block(uint32_t block_num) override {
      amqp_publisher_->publish_messages_raw(std::move(queue_));
      queue_.clear();
   }

   void publish(const std::vector<char>& data, const eosio::name& routing_key) override {
      if (exchange_name_.empty()) {
         queue_.emplace_back(std::make_pair(queue_name_, data));
      } else {
         queue_.emplace_back(std::make_pair(routing_key.to_string(), data));
      }
   }

 private:

   void init(boost::asio::io_service& io_service, const AMQP::Address& address) {
      handler_    = std::make_shared<rabbitmq_handler>(io_service);
      connection_ = std::make_shared<AMQP::TcpConnection>(handler_.get(), address);
      channel_    = std::make_shared<AMQP::TcpChannel>(connection_.get());
   }

   void declare_queue() {
      auto& queue = channel_->declareQueue( queue_name_, AMQP::durable);
      queue.onSuccess([this](const std::string& name, uint32_t messagecount, uint32_t consumercount) {
         ilog("RabbitMQ declare queue Successfully!\n Queue ${q} - Messages: ${mc} - Consumers: ${cc}",
              ("q", name)("mc", messagecount)("cc", consumercount));
         connection_->close();
         amqp_publisher_ = std::make_shared<eosio::reliable_amqp_publisher>(address_, "", "", unconfirmed_path_);
      });
      queue.onError([](const char* error_message) {
         throw std::runtime_error("RabbitMQ Queue error: " + std::string(error_message));
      });
   }

   void declare_exchange(const std::string& exchange_type) {
      auto type = AMQP::direct;
      if (exchange_type == "fanout") {
         type = AMQP::fanout;
      } else if (exchange_type != "direct") {
         throw std::runtime_error("Unsupported RabbitMQ exchange type: " + exchange_type);
      }

      auto& exchange = channel_->declareExchange( exchange_name_, type);
      exchange.onSuccess( [this]() {
         ilog( "RabbitMQ declare exchange Successfully!\n Exchange ${e}", ("e", exchange_name_) );
         connection_->close();
         amqp_publisher_ = std::make_shared<eosio::reliable_amqp_publisher>( address_, exchange_name_, "", unconfirmed_path_ );
      } );
      exchange.onError([](const char* error_message) {
         throw std::runtime_error("RabbitMQ Exchange error: " + std::string(error_message));
      });
   }
};

class rabbitmq_handler : public AMQP::LibBoostAsioHandler {
 public:
   explicit rabbitmq_handler(boost::asio::io_service& io_service) : AMQP::LibBoostAsioHandler(io_service) {}

   void onError(AMQP::TcpConnection* connection, const char* message) {
      throw std::runtime_error("rabbitmq connection failed: " + std::string(message));
   }

   uint16_t onNegotiate(AMQP::TcpConnection* connection, uint16_t interval) { return 0; }
};

// Parse the specified argument of a '--stream-rabbits'
// or '--stream-rabbits-exchange' option and split it into:
//
// - RabbitMQ address, returned as an instance of AMQP::Address;
// - (optional) queue name or exchange specification, saved to
//   the output argument 'queue_name_or_exchange_spec';
// - (optional) RabbitMQ routes, saved to the output argument 'routes'.
//
// Because all of the above fields use slashes as separators, the following
// precedence rules are applied when parsing:
//
// Input                   Output
// ------------------      ----------------------------------------
// amqp://a                host='a' vhost='' queue='' routes=[]
// amqp://a/b              host='a' vhost='' queue='b' routes=[]
// amqp://a/b/c            host='a' vhost='' queue='b' routes='c'.split(',')
// amqp://a/b/c/d          host='a' vhost='b' queue='c' routes='d'.split(',')
//
// To specify a vhost without specifying a queue name or routes, omit
// the queue name and use an asterisk or an empty string for the routes,
// like so:
//
//    amqp://host/vhost//*
//    amqp:///vhost//*
//
inline AMQP::Address parse_rabbitmq_address(const std::string& cmdline_arg, std::string& queue_name_or_exchange_spec,
                                            std::vector<eosio::name>& routes) {
   // AMQP address starts with "amqp://" or "amqps://".
   const auto double_slash_pos = cmdline_arg.find("//");
   if (double_slash_pos == std::string::npos) {
      // Invalid RabbitMQ address - AMQP::Address constructor
      // will throw an exception.
      return AMQP::Address(cmdline_arg);
   }

   const auto first_slash_pos = cmdline_arg.find('/', double_slash_pos + 2);
   if (first_slash_pos == std::string::npos) {
      return AMQP::Address(cmdline_arg);
   }

   const auto second_slash_pos = cmdline_arg.find('/', first_slash_pos + 1);
   if (second_slash_pos == std::string::npos) {
      queue_name_or_exchange_spec = cmdline_arg.substr(first_slash_pos + 1);
      return AMQP::Address(cmdline_arg.substr(0, first_slash_pos));
   }

   const auto third_slash_pos = cmdline_arg.find('/', second_slash_pos + 1);
   if (third_slash_pos == std::string::npos) {
      queue_name_or_exchange_spec = cmdline_arg.substr(first_slash_pos + 1, second_slash_pos - (first_slash_pos + 1));
      routes                      = extract_routes(cmdline_arg.substr(second_slash_pos + 1));
      return AMQP::Address(cmdline_arg.substr(0, first_slash_pos));
   }

   queue_name_or_exchange_spec = cmdline_arg.substr(second_slash_pos + 1, third_slash_pos - (second_slash_pos + 1));
   routes                      = extract_routes(cmdline_arg.substr(third_slash_pos + 1));
   return AMQP::Address(cmdline_arg.substr(0, second_slash_pos));
}

inline void initialize_rabbits_queue(boost::asio::io_service&                      io_service,
                                     std::vector<std::unique_ptr<stream_handler>>& streams,
                                     const std::vector<std::string>&               rabbits,
                                     const boost::filesystem::path&                p) {
   for (const std::string& rabbit : rabbits) {
      std::string              queue_name;
      std::vector<eosio::name> routes;

      AMQP::Address address = parse_rabbitmq_address(rabbit, queue_name, routes);


      if (queue_name.empty()) {
         queue_name = "stream.default";
      }

      // TODO: uniqueness
      boost::filesystem::path msg_path = p / ("queue_" + queue_name) / "stream.bin";

      streams.emplace_back(std::make_unique<rabbitmq>(io_service, std::move(routes), address, std::move(queue_name), msg_path));
   }
}

inline void initialize_rabbits_exchange(boost::asio::io_service&                      io_service,
                                        std::vector<std::unique_ptr<stream_handler>>& streams,
                                        const std::vector<std::string>&               rabbits,
                                        const boost::filesystem::path&                p) {
   for (const std::string& rabbit : rabbits) {
      std::string              exchange;
      std::vector<eosio::name> routes;

      AMQP::Address address = parse_rabbitmq_address(rabbit, exchange, routes);

      std::string exchange_type;

      const auto double_column_pos = exchange.find("::");
      if (double_column_pos != std::string::npos) {
         exchange_type = exchange.substr(double_column_pos + 2);
         exchange.erase(double_column_pos);
      }

      // TODO: uniqueness
      boost::filesystem::path msg_path = p / ("exchange_" + exchange) / "stream.bin";

      streams.emplace_back(std::make_unique<rabbitmq>(io_service, std::move(routes), address, std::move(exchange),
                                                      std::move(exchange_type), msg_path));
   }
}

} // namespace b1
