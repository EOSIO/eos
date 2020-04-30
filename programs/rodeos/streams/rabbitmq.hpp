#pragma once

#include "stream.hpp"
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <fc/log/logger.hpp>

namespace b1 {

class rabbitmq : public stream_handler {
   AmqpClient::Channel::ptr_t channel_;
   std::string                name_;
   std::vector<eosio::name>   routes_;

 public:
   rabbitmq(std::vector<eosio::name> routes, std::string host, int port, std::string user, std::string password,
            std::string name)
       : name_(name), routes_(routes) {
      ilog("connecting AMQP...");
      channel_ = AmqpClient::Channel::Create(host, port, user, password);
      ilog("AMQP connected");
      channel_->DeclareQueue(name, false, true, false, false);
   }

   std::vector<eosio::name>& get_routes() { return routes_; }

   void publish(const char* data, uint64_t data_size) {
      std::string message(data, data_size);
      auto        amqp_message = AmqpClient::BasicMessage::Create(message);
      channel_->BasicPublish("", name_, amqp_message);
   }
};

inline void initialize_rabbits(std::vector<std::unique_ptr<stream_handler>>& streams,
                               const std::vector<std::string>&               rabbits) {
   for (auto rabbit : rabbits) {
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

      std::string user     = "guest";
      std::string password = "guest";
      pos                  = rabbit.find("@");
      if (pos != std::string::npos) {
         auto auth_pos = rabbit.substr(0, pos).find(":");
         user          = rabbit.substr(0, auth_pos);
         password      = rabbit.substr(auth_pos + 1, pos - auth_pos - 1);
         rabbit.erase(0, pos + 1);
      }

      pos              = rabbit.find(":");
      std::string host = rabbit.substr(0, pos);
      int         port = std::stoi(rabbit.substr(pos + 1, rabbit.length()));

      ilog("adding rabbitmq stream ${h}:${p} -- queue: ${queue} | auth: ${user}/****",
           ("h", host)("p", port)("queue", queue_name)("user", user));
      rabbitmq rmq{ routings, host, port, user, password, queue_name };
      streams.emplace_back(std::make_unique<rabbitmq>(rmq));
   }
}

} // namespace b1
