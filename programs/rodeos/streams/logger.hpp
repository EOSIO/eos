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

   void publish(const char* data, uint64_t data_size, const eosio::name& routing_key) override {
      ilog("logger stream [${data_size}] >> ${data}", ("data", data)("data_size", data_size));
   }
};

inline void initialize_loggers(std::vector<std::unique_ptr<stream_handler>>& streams,
                               const std::vector<std::string>&               loggers) {
   for (const auto& routes_str : loggers) {
      std::vector<eosio::name> routes = extract_routes(routes_str);
      logger                   logger_streamer{ std::move(routes) };
      streams.emplace_back(std::make_unique<logger>(std::move(logger_streamer)));
   }
}

} // namespace b1
