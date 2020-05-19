#pragma once

#include "stream.hpp"
#include <fc/log/logger.hpp>

namespace b1 {

class logger : public stream_handler {
   std::vector<eosio::name> routes_;

 public:
   logger(std::vector<eosio::name> routes) : routes_(std::move(routes)) {
      ilog("logger initialized");
   }

   const std::vector<eosio::name>& get_routes() const override { return routes_; }

   void publish(const char* data, uint64_t data_size) override {
      ilog("logger stream [${data_size}] >> ${data}", ("data", data)("data_size", data_size));
   }
};

inline void initialize_loggers(std::vector<std::unique_ptr<stream_handler>>& streams,
                               const std::vector<std::string>&               loggers) {
   for (const auto& routings : loggers) {
      std::vector<eosio::name> routing_keys = extract_routings(routings);
      logger                   logger_streamer{ std::move(routing_keys) };
      streams.emplace_back(std::make_unique<logger>(std::move(logger_streamer)));
   }
}

} // namespace b1
