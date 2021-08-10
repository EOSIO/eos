#pragma once
#include <eosio/abi.hpp>
#include <fc/log/logger.hpp>

namespace b1 {

struct streamer_t {
   virtual ~streamer_t() {}
   virtual void start_block(uint32_t block_num) {};
   virtual void stream_data(const char* data, uint64_t data_size) = 0;
   virtual void stop_block(uint32_t block_num) {}
};

class stream_handler {
 public:
   explicit stream_handler(std::vector<std::string> routes)
   : routes_(std::move(routes)) {}

   virtual ~stream_handler() {}
   virtual void start_block(uint32_t block_num) {};
   virtual void publish(const std::vector<char>& data, const std::string& routing_key) = 0;
   virtual void stop_block(uint32_t block_num) {}

   bool check_route(const std::string& stream_route) {
      if (routes_.size() == 0) {
         return true;
      }

      for (const auto& name : routes_) {
         if (name == stream_route) {
            return true;
         }
      }

      return false;
   }

private:
   std::vector<std::string> routes_;
};

inline std::vector<std::string> extract_routes(const std::string& routes_str) {
   std::vector<std::string> streaming_routes{};
   bool                     star     = false;
   std::string              routings = routes_str;
   while (routings.size() > 0) {
      size_t      pos          = routings.find(",");
      size_t      route_length = pos == std::string::npos ? routings.length() : pos;
      std::string route        = routings.substr(0, pos);
      ilog("extracting route ${route}", ("route", route));
      if (route != "*") {
         streaming_routes.emplace_back(std::move(route));
      } else {
         star = true;
      }
      routings.erase(0, route_length + 1);
   }
   if (star && !streaming_routes.empty()) {
      throw std::runtime_error(std::string("Invalid routes '") + routes_str + "'");
   }
   return streaming_routes;
}

} // namespace b1
