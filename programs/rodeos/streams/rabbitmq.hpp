#pragma once

#include "amqpcpp.h"
#include "amqpcpp/libboostasio.h"
#include "amqpcpp/linux_tcp.h"
#include "stream.hpp"
#include <appbase/application.hpp>
#include <eosio/amqp/amqp_handler.hpp>
#include <eosio/amqp/reliable_amqp_publisher.hpp>
#include <fc/log/logger.hpp>
#include <boost/filesystem/operations.hpp>
#include <memory>

namespace b1 {

class rabbitmq : public stream_handler {
   std::shared_ptr<eosio::reliable_amqp_publisher> amqp_publisher_;
   const AMQP::Address                  address_;
   const bool                           publish_immediately_ = false;
   const std::string                    exchange_name_;
   const std::string                    queue_name_;
   const std::vector<eosio::name>       routes_;
   const boost::filesystem::path        unconfirmed_path_;
   // capture all messages per block and send as one amqp transaction
   std::deque<std::pair<std::string, std::vector<char>>> queue_;

 public:
   rabbitmq(std::vector<eosio::name> routes, const AMQP::Address& address, bool publish_immediately,
            std::string queue_name, const boost::filesystem::path& unconfirmed_path)
       : address_(address)
       , publish_immediately_(publish_immediately)
       , queue_name_( std::move( queue_name))
       , routes_( std::move( routes))
       , unconfirmed_path_( unconfirmed_path)
   {
      ilog("Connecting to RabbitMQ address ${a} - Queue: ${q}...", ("a", address)( "q", queue_name_));
      bool error = false;
      eosio::amqp declare_queue( address_, queue_name_, [&error](const std::string& err){
         elog("AMQP Queue error: ${e}", ("e", err));
         appbase::app().quit();
         error = true;
      } );
      if( error ) return;
      amqp_publisher_ = std::make_shared<eosio::reliable_amqp_publisher>(address_, "", "", unconfirmed_path_);
   }

   rabbitmq(std::vector<eosio::name> routes, const AMQP::Address& address, bool publish_immediately,
            std::string exchange_name, std::string exchange_type, const boost::filesystem::path& unconfirmed_path)
       : address_(address)
       , publish_immediately_(publish_immediately)
       , exchange_name_( std::move( exchange_name))
       , routes_( std::move( routes))
       , unconfirmed_path_( unconfirmed_path)
   {
      ilog("Connecting to RabbitMQ address ${a} - Exchange: ${e}...", ("a", address)( "e", exchange_name_));
      bool error = false;
      eosio::amqp declare_exchange( address_, exchange_name_, exchange_type, [&error](const std::string& err){
         elog("AMQP Exchange error: ${e}", ("e", err));
         appbase::app().quit();
         error = true;
      } );
      if( error ) return;
      amqp_publisher_ = std::make_shared<eosio::reliable_amqp_publisher>( address_, exchange_name_, "", unconfirmed_path_ );
   }

   const std::vector<eosio::name>& get_routes() const override { return routes_; }

   void start_block(uint32_t block_num) override {
      queue_.clear();
   }

   void stop_block(uint32_t block_num) override {
      if( !publish_immediately_ ) {
         amqp_publisher_->publish_messages_raw( std::move( queue_ ) );
         queue_.clear();
      }
   }

   void publish(const std::vector<char>& data, const eosio::name& routing_key) override {
      if (exchange_name_.empty()) {
         if( publish_immediately_ ) {
            amqp_publisher_->publish_message_direct( queue_name_, data );
         } else {
            queue_.emplace_back( std::make_pair( queue_name_, data ) );
         }
      } else {
         if( publish_immediately_ ) {
            amqp_publisher_->publish_message_direct( routing_key.to_string(), data );
         } else {
            queue_.emplace_back( std::make_pair( routing_key.to_string(), data ) );
         }
      }
   }

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

inline void initialize_rabbits_queue(std::vector<std::unique_ptr<stream_handler>>& streams,
                                     const std::vector<std::string>&               rabbits,
                                     bool                                          publish_immediately,
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

      streams.emplace_back(std::make_unique<rabbitmq>(std::move(routes), address, publish_immediately,
                                                      std::move(queue_name), msg_path));
   }
}

inline void initialize_rabbits_exchange(std::vector<std::unique_ptr<stream_handler>>& streams,
                                        const std::vector<std::string>&               rabbits,
                                        bool                                          publish_immediately,
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

      streams.emplace_back(std::make_unique<rabbitmq>(std::move(routes), address, publish_immediately,
                                                      std::move(exchange), std::move(exchange_type), msg_path));
   }
}

} // namespace b1
