#pragma once

#include "stream.hpp"
#include "amqpcpp.h"
#include "amqpcpp/libboostasio.h"
#include "amqpcpp/linux_tcp.h"
#include <fc/log/logger.hpp>
#include <memory>

namespace b1 {

class rabbitmq_handler;

class rabbitmq : public stream_handler {
   std::shared_ptr<rabbitmq_handler>    handler_;
   std::shared_ptr<AMQP::TcpConnection> connection_;
   std::shared_ptr<AMQP::TcpChannel>    channel_;
   std::string                          name_;
   std::vector<eosio::name>             routes_;

 public:
   rabbitmq(boost::asio::io_service& io_service, std::vector<eosio::name> routes, std::string address, std::string name)
       : name_(name), routes_(routes) {
      AMQP::Address amqp_address(address);
      ilog("Connecting to RabbitMQ address ${a} - Queue: ${q}...", ("a", std::string(amqp_address))("q", name));

      handler_    = std::make_shared<rabbitmq_handler>(io_service);
      connection_ = std::make_shared<AMQP::TcpConnection>(handler_.get(), amqp_address);
      channel_    = std::make_shared<AMQP::TcpChannel>(connection_.get());
      declare_queue();
   }

   std::vector<eosio::name>& get_routes() { return routes_; }

   void publish(const char* data, uint64_t data_size) { channel_->publish("", name_, data, data_size, 0); }

 private:
   void declare_queue() {
      auto& queue = channel_->declareQueue(name_, AMQP::durable);
      queue.onSuccess([](const std::string& name, uint32_t messagecount, uint32_t consumercount) {
         ilog("RabbitMQ Connected Successfully!\n Queue ${q} - Messages: ${mc} - Consumers: ${cc}",
              ("q", name)("mc", messagecount)("cc", consumercount));
      });
      queue.onError([](const char* error_message) {
         throw std::runtime_error("RabbitMQ Queue error: " + std::string(error_message));
      });
   }
};

class rabbitmq_handler : public AMQP::LibBoostAsioHandler {
 public:
   rabbitmq_handler(boost::asio::io_service& io_service) : AMQP::LibBoostAsioHandler(io_service) {}

   void onError(AMQP::TcpConnection* connection, const char* message) {
      throw std::runtime_error("rabbitmq connection failed: " + std::string(message));
   }
};

inline void initialize_rabbits(boost::asio::io_service&                      io_service,
                               std::vector<std::unique_ptr<stream_handler>>& streams,
                               const std::vector<std::string>&               rabbits) {
   for (auto rabbit : rabbits) {
      rabbit            = rabbit.substr(7, rabbit.length());
      size_t pos        = rabbit.find("/");
      size_t pos_router = rabbit.find_last_of("/");
      bool   has_router = pos_router != pos;

      std::vector<eosio::name> routings = {};
      if (has_router) {
         routings = extract_routings(rabbit.substr(pos_router + 1, rabbit.length()));
         rabbit.erase(pos_router, rabbit.length());
      }

      std::string queue_name = "stream.default";
      if (pos != std::string::npos) {
         queue_name = rabbit.substr(pos + 1, rabbit.length());
         rabbit.erase(pos, rabbit.length());
      }

      std::string address = "amqp://" + rabbit;
      rabbitmq rmq{ io_service, routings, address, queue_name };
      streams.emplace_back(std::make_unique<rabbitmq>(rmq));
   }
}

} // namespace b1
