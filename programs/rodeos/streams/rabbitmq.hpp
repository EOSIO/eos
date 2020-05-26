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
   std::string                          exchangeName_;
   std::string                          queueName_;
   std::vector<eosio::name>             routes_;

 public:
   rabbitmq(boost::asio::io_service& io_service, std::vector<eosio::name> routes, std::string address, std::string queue_name)
       : queueName_(std::move(queue_name)), routes_(std::move(routes)) {
      AMQP::Address amqp_address(address);
      ilog("Connecting to RabbitMQ address ${a} - Queue: ${q}...", ("a", std::string(amqp_address))("q", queueName_));

      init(io_service, amqp_address);

      exchangeName_ = DEFAULT_EXCHANGE;
      declare_queue();
   }

   rabbitmq(boost::asio::io_service& io_service, std::string address, std::string exchange_name, std::string exchange_type)
           : exchangeName_(std::move(exchange_name))) {
      AMQP::Address amqp_address(address);
      ilog("Connecting to RabbitMQ address ${a} - Exchange: ${e}...", ("a", std::string(amqp_address))("e", exchangeName_));

      init(io_service, amqp_address);

      declare_exchange(exchange_type);
   }

   const std::vector<eosio::name>& get_routes() const override { return routes_; }

   void publish(const char* data, uint64_t data_size, const std::string& route) override {
      if (DEFAULT_EXCHANGE == exchangeName_) {
         channel_->publish(exchangeName_, queueName_, data, data_size, 0);
      } else {
         channel_->publish(exchangeName_, route, data, data_size, 0);
      }
   }

 private:
   inline static const string DEFAULT_EXCHANGE = "";

   void init(boost::asio::io_service& io_service, AMQP::Address& amqp_address) {
      handler_    = std::make_shared<rabbitmq_handler>(io_service);
      connection_ = std::make_shared<AMQP::TcpConnection>(handler_.get(), amqp_address);
      channel_    = std::make_shared<AMQP::TcpChannel>(connection_.get());
   }

   void declare_queue() {
      auto& queue = channel_->declareQueue(queueName_, AMQP::durable);
      queue.onSuccess([](const std::string& name, uint32_t messagecount, uint32_t consumercount) {
         ilog("RabbitMQ Connected Successfully!\n Queue ${q} - Messages: ${mc} - Consumers: ${cc}",
              ("q", name)("mc", messagecount)("cc", consumercount));
      });
      queue.onError([](const char* error_message) {
         throw std::runtime_error("RabbitMQ Queue error: " + std::string(error_message));
      });
   }

   void declare_exchange(std::string& exchange_type) {
      auto type = AMQP::direct;
      switch(exchange_type) {
         case "fanout"  :
            type = AMQP::fanout;
            break;
         case "topic"  :
            type = AMQP::topic;
            break;
      }

      auto& exchange = channel_->declareExchange(exchangeName_, type);
      exchange.onSuccess([](const std::string& name, uint32_t messagecount, uint32_t consumercount) {
         ilog("RabbitMQ Connected Successfully!\n Exchange ${e} - Messages: ${mc} - Consumers: ${cc}",
              ("e", name)("mc", messagecount)("cc", consumercount));
      });
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

inline void initialize_rabbits(boost::asio::io_service&                      io_service,
                               std::vector<std::unique_ptr<stream_handler>>& streams,
                               const std::vector<std::string>&               rabbits) {
   for (std::string rabbit : rabbits) {
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
      rabbitmq rmq{ io_service, std::move(routings), std::move(address), std::move(queue_name) };
      streams.emplace_back(std::make_unique<rabbitmq>(std::move(rmq)));
   }
}

   inline void initialize_rabbits_exchange(boost::asio::io_service&                      io_service,
                                           std::vector<std::unique_ptr<stream_handler>>& streams,
                                           const std::vector<std::string>&               rabbits) {
      for (std::string rabbit : rabbits) {
         rabbit            = rabbit.substr(7, rabbit.length());
         size_t pos        = rabbit.find("/");
         size_t pos_exchange_type = rabbit.find_last_of("::");
         bool   has_exchange_type = pos_exchange_type != pos;

         std::string exchange_type = "";
         if (has_exchange_type) {
            exchange_type = rabbit.substr(pos_router + 1, rabbit.length());
            rabbit.erase(pos_router, rabbit.length());
         }

         std::string exchange_name = "";
         if (pos != std::string::npos) {
            exchange_name = rabbit.substr(pos + 1, rabbit.length());
            rabbit.erase(pos, rabbit.length());
         }

         std::string address = "amqp://" + rabbit;
         rabbitmq rmq{ io_service, std::move(address), std::move(exchange_name), std::move(exchange_type) };
         streams.emplace_back(std::make_unique<rabbitmq>(std::move(rmq)));
      }
   }

} // namespace b1
