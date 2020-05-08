#pragma once

#include "stream.hpp"
// #include <SimpleAmqpClient/SimpleAmqpClient.h>
#include "amqpcpp.h"
#include "amqpcpp/linux_tcp.h"
#include <fc/log/logger.hpp>
#include <memory>

namespace b1 {

class rabbitmq_handler : public AMQP::TcpHandler {
   /**
     *  Method that is called by the AMQP-CPP library when it wants to interact
     *  with the main event loop. The AMQP-CPP library is completely non-blocking,
     *  and only make "write()" or "read()" system calls when it knows in advance
     *  that these calls will not block. To register a filedescriptor in the
     *  event loop, it calls this "monitor()" method with a filedescriptor and
     *  flags telling whether the filedescriptor should be checked for readability
     *  or writability.
     *
     *  @param  connection      The connection that wants to interact with the event loop
     *  @param  fd              The filedescriptor that should be checked
     *  @param  flags           Bitwise or of AMQP::readable and/or AMQP::writable
     */
    virtual void monitor(AMQP::TcpConnection *connection, int fd, int flags) override {
        // @todo
        //  add your own implementation, for example by adding the file
        //  descriptor to the main application event loop (like the select() or
        //  poll() loop). When the event loop reports that the descriptor becomes
        //  readable and/or writable, it is up to you to inform the AMQP-CPP
        //  library that the filedescriptor is active by calling the
        //  connection->process(fd, flags) method.
        connection->process(fd, flags);
    }
};

class rabbitmq : public stream_handler {
   // AmqpClient::Channel::ptr_t channel_;
   std::shared_ptr<AMQP::TcpChannel>    channel_;
   std::shared_ptr<AMQP::TcpConnection> connection_;
   rabbitmq_handler                  handler_;
   std::string                       name_;
   std::vector<eosio::name>          routes_;

 public:
   rabbitmq(std::vector<eosio::name> routes, std::string host, int port, std::string user, std::string password,
            std::string name)
       : name_(name), routes_(routes) {
      ilog("connecting AMQP...");
      host = "amqp://" + host;
      AMQP::Address amqp_address(host, port, AMQP::Login(user, password), "/");
      connection_ = std::make_shared<AMQP::TcpConnection>(&handler_, amqp_address);

      channel_ = std::make_shared<AMQP::TcpChannel>(connection_.get());
      name_    = "euqmando";
      channel_->declareQueue(name_).onSuccess([](const std::string& name, uint32_t mc, uint32_t cc) {
         ilog("queue ${n} declared! mc ${mc} // cc ${cc}", ("n", name)("mc", mc)("cc", cc));
      });
      ilog("AMQP-CPPTCP connected");
      // channel_ = AmqpClient::Channel::Create(host, port, user, password);
      // channel_->DeclareQueue(name, false, true, false, false);
   }

   std::vector<eosio::name>& get_routes() { return routes_; }

   void publish(const char* data, uint64_t data_size) {
      // std::string message(data, data_size);
      // auto        amqp_message = AmqpClient::BasicMessage::Create(message);
      // channel_->BasicPublish("", name_, amqp_message);
      channel_->publish("", name_, data, data_size, 0);
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
