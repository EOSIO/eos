// copyright defined in LICENSE.txt

#include "streamer_plugin.hpp"
#include "streams/logger.hpp"
#include "streams/rabbitmq.hpp"
#include "streams/stream.hpp"
#include <abieos.hpp>
#include <eosio/abi.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/trace.hpp>
#include <boost/filesystem/operations.hpp>
#include <memory>

namespace b1 {

using namespace appbase;
using namespace std::literals;

struct streamer_plugin_impl : public streamer_t {

   void start_block(uint32_t block_num) override {
      auto blk_trace = fc_create_trace("Block");
      auto blk_span = fc_create_span(blk_trace, "Filter-Start");
      fc_add_str_tag( blk_span, "block_num", std::to_string( block_num ) );

      for (const auto& stream : streams) {
         stream->start_block(block_num);
      }
   }

   void stream_data(const char* data, uint64_t data_size) override {
      eosio::input_stream bin(data, data_size);
      stream_wrapper      res = eosio::from_bin<stream_wrapper>(bin);
      const auto&         sw  = std::get<stream_wrapper_v0>(res);
      publish_to_streams(sw);
   }

   void publish_to_streams(const stream_wrapper_v0& sw) {
      for (const auto& stream : streams) {
         if (stream->check_route(sw.route)) {
            stream->publish(sw.data, sw.route);
         }
      }
   }

   void stop_block(uint32_t block_num) override {
      auto blk_trace = fc_create_trace("Block");
      auto blk_span = fc_create_span(blk_trace, "Filter-Stop");
      fc_add_str_tag( blk_span, "block_num", std::to_string( block_num ) );

      for (const auto& stream : streams) {
         stream->stop_block(block_num);
      }
   }

   std::vector<std::unique_ptr<stream_handler>> streams;
   bool delete_previous = false;
   bool publish_immediately = false;
};

static abstract_plugin& _streamer_plugin = app().register_plugin<streamer_plugin>();

streamer_plugin::streamer_plugin() : my(std::make_shared<streamer_plugin_impl>()) {}

streamer_plugin::~streamer_plugin() {}

void streamer_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("stream-rabbits", bpo::value<std::vector<string>>()->composing(),
      "RabbitMQ Streams to queues if any; Format: amqp://USER:PASSWORD@ADDRESS:PORT/QUEUE[/STREAMING_ROUTE, ...]");
   op("stream-rabbits-exchange", bpo::value<std::vector<string>>()->composing(),
      "RabbitMQ Streams to exchanges if any; Format: amqp://USER:PASSWORD@ADDRESS:PORT/EXCHANGE[::EXCHANGE_TYPE][/STREAMING_ROUTE, ...]");
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
         auto rabbits = options.at("stream-rabbits").as<std::vector<std::string>>();
         initialize_rabbits_queue(my->streams, rabbits, my->publish_immediately, stream_data_path);
      }

      if (options.count("stream-rabbits-exchange")) {
         auto rabbits = options.at("stream-rabbits-exchange").as<std::vector<std::string>>();
         initialize_rabbits_exchange(my->streams, rabbits, my->publish_immediately, stream_data_path);
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
