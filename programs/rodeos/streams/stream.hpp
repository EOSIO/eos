#pragma once
#include <eosio/abi.hpp>
#include <fc/log/logger.hpp>

namespace b1 {

class stream_handler {
 public:
   virtual ~stream_handler() {}
   virtual std::vector<eosio::name>& get_routes()                                  = 0;
   virtual void                      publish(const char* data, uint64_t data_size) = 0;

   virtual bool check_route(const eosio::name& stream_route) {
      if (get_routes().size() == 0) {
         return true;
      }

      for (const auto& name : get_routes()) {
         if (name == stream_route) {
            return true;
         }
      }

      return false;
   }
};

inline std::vector<eosio::name> extract_routings(std::string routings) {
   std::vector<eosio::name> routing_keys{};
   while (routings.size() > 0) {
      size_t      pos          = routings.find(",");
      size_t      route_length = pos == std::string::npos ? routings.length() : pos;
      std::string route        = routings.substr(0, pos);
      ilog("extracting route ${route}", ("route", route));
      if (route != "*") {
         routing_keys.emplace_back(eosio::name(route));
      }
      routings.erase(0, route_length + 1);
   }
   return routing_keys;
}

} // namespace b1
