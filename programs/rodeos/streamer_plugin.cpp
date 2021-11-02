// copyright defined in LICENSE.txt

#include "streamer_plugin.hpp"
#include "streams/logger.hpp"
#include "streams/rabbitmq.hpp"
#include "streams/stream.hpp"
#include <abieos.hpp>
#include <eosio/abi.hpp>
#include <eosio/chain/exceptions.hpp>
#include <chainbase/pinnable_mapped_file.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/trace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <memory>

namespace b1 {

using namespace appbase;
using namespace std::literals;

struct streamer_plugin_impl : public streamer_t {

   void start_block(uint32_t block_num) override {
      for (const auto& stream : streams) {
         stream->start_block(block_num);
      }
   }

   void stream_data(const char* data, uint64_t data_size) override {
      eosio::input_stream bin(data, data_size);
      stream_wrapper      res = eosio::from_bin<stream_wrapper>(bin);
      std::visit([&](const auto& sw) { publish_to_streams(sw); }, res);
   }

   void publish_to_streams(const stream_wrapper_v0& sw) {
      std::string route;
      for (const auto& stream : streams) {
         route = sw.route.to_string();
         if (stream->check_route(route)) {
            stream->publish(sw.data, route);
         }
      }
   }

   void publish_to_streams(const stream_wrapper_v1& sw) {
      for (const auto& stream : streams) {
         if (stream->check_route(sw.route)) {
            stream->publish(sw.data, sw.route);
         }
      }
   }

   void stop_block(uint32_t block_num) override {
      for (const auto& stream : streams) {
         stream->stop_block(block_num);
      }
   }

   std::vector<std::unique_ptr<stream_handler>> streams;
   bool delete_previous = false;
   bool publish_immediately = false;
};

static abstract_plugin& _streamer_plugin = app().register_plugin<streamer_plugin>();

streamer_plugin::streamer_plugin() : my(std::make_shared<streamer_plugin_impl>()) {
   app().register_config_type<chainbase::pinnable_mapped_file::map_mode>();
}

streamer_plugin::~streamer_plugin() {}

void streamer_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();

   std::string rabbits_default_value;
   char* rabbits_env_var = std::getenv(EOSIO_STREAM_RABBITS_ENV_VAR);
   if (rabbits_env_var) rabbits_default_value = rabbits_env_var;
   op("stream-rabbits", bpo::value<std::string>()->default_value(rabbits_default_value),
      "Addresses of RabbitMQ queues to stream to. Format: amqp://USER:PASSWORD@ADDRESS:PORT/QUEUE[/STREAMING_ROUTE, ...]. "
      "Multiple queue addresses can be specified with ::: as the delimiter, such as \"amqp://u1:p1@amqp1:5672/queue1:::amqp://u2:p2@amqp2:5672/queue2\"."
      "If this option is not specified, the value from the environment variable "
      EOSIO_STREAM_RABBITS_ENV_VAR
      " will be used.");

   std::string rabbits_exchange_default_value;
   char* rabbits_exchange_env_var = std::getenv(EOSIO_STREAM_RABBITS_EXCHANGE_ENV_VAR);
   if (rabbits_exchange_env_var) rabbits_exchange_default_value = rabbits_exchange_env_var;
   op("stream-rabbits-exchange", bpo::value<std::string>()->default_value(rabbits_exchange_default_value),
      "Addresses of RabbitMQ exchanges to stream to. amqp://USER:PASSWORD@ADDRESS:PORT/EXCHANGE[::EXCHANGE_TYPE][/STREAMING_ROUTE, ...]. "
      "Multiple queue addresses can be specified with ::: as the delimiter, such as \"amqp://u1:p1@amqp1:5672/exchange1:::amqp://u2:p2@amqp2:5672/exchange2\"."
      "If this option is not specified, the value from the environment variable "
      EOSIO_STREAM_RABBITS_EXCHANGE_ENV_VAR
      " will be used.");

   op("stream-rabbits-immediately", bpo::bool_switch(&my->publish_immediately)->default_value(false),
      "Stream to RabbitMQ immediately instead of batching per block. Disables reliable message delivery.");
   op("stream-loggers", bpo::value<std::vector<string>>()->composing(),
      "Logger Streams if any; Format: [routing_keys, ...]");

   cli.add_options()
     ("stream-delete-unsent", bpo::bool_switch(&my->delete_previous),
      "Delete unsent AMQP stream data retained from previous connections");
}

void streamer_plugin::plugin_initialize(const variables_map& options) {
   try {
      const boost::filesystem::path stream_data_path = appbase::app().data_dir() / "stream";

      if( my->delete_previous ) {
         if( boost::filesystem::exists( stream_data_path ) )
            boost::filesystem::remove_all( stream_data_path );
      }

      if (options.count("stream-loggers")) {
         auto loggers = options.at("stream-loggers").as<std::vector<std::string>>();
         initialize_loggers(my->streams, loggers);
      }

      if (options.count("stream-rabbits")) {
         std::vector<std::string> rabbits;
         boost::split(rabbits,
            options.at("stream-rabbits").as<std::string>(),
            boost::algorithm::is_any_of(":::"),
            boost::algorithm::token_compress_on);
         initialize_rabbits_queue(my->streams, rabbits, my->publish_immediately, stream_data_path);
      }

      if (options.count("stream-rabbits-exchange")) {
         std::vector<std::string> rabbits_exchanges;
         boost::split(rabbits_exchanges,
            options.at("stream-rabbits-exchange").as<std::string>(),
            boost::algorithm::is_any_of(":::"),
            boost::algorithm::token_compress_on);
         initialize_rabbits_exchange(my->streams, rabbits_exchanges, my->publish_immediately, stream_data_path);
      }

      ilog("initialized streams: ${streams}", ("streams", my->streams.size()));
   } FC_LOG_AND_RETHROW()
}

void streamer_plugin::plugin_startup() {
   try {
      cloner_plugin* cloner = app().find_plugin<cloner_plugin>();
      EOS_ASSERT( cloner, eosio::chain::plugin_config_exception, "cloner_plugin not found" );
      cloner->set_streamer( my );
   } FC_LOG_AND_RETHROW()
}

void streamer_plugin::plugin_shutdown() {}

} // namespace b1
