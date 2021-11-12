#pragma once

#include "stream.hpp"
#include <fc/log/logger.hpp>

namespace b1 {

class logger : public stream_handler {
 public:
   explicit logger(std::vector<std::string> routes)
   : stream_handler(std::move(routes)) {
      ilog("logger initialized");
   }

   void publish(const std::vector<char>& data, const std::string& routing_key) override {
      ilog("logger stream ${r}: [${data_size}] >> ${data}",
           ("r", routing_key)("data", fc::variant(data).as_string())("data_size", data.size()));
   }
};

inline void initialize_loggers(std::vector<std::unique_ptr<stream_handler>>& streams,
                               const std::vector<std::string>&               loggers) {
   for (const auto& routes_str : loggers) {
      std::vector<std::string> routes = extract_routes(routes_str);
      logger                   logger_streamer{ std::move(routes) };
      streams.emplace_back(std::make_unique<logger>(std::move(logger_streamer)));
   }
}

} // namespace b1
