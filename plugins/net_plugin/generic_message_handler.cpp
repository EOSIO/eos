#include <eosio/chain/types.hpp>

#include <eosio/net_plugin/generic_message_handler.hpp>

namespace eosio { namespace net {

   generic_message_handler::generic_router::generic_router(const std::type_info& type)
      : type(type) {
   }

   void generic_message_handler::route(const generic_message& msg, const string& endpoint) const {
      auto existing_router = _routers.find(msg.type);
      if (existing_router == _routers.cend()) {
         dlog( "No routes for generic_message of type: ${type}", ("type", msg.type) );
         return;
      }

      existing_router->second->route(msg, endpoint);
   }

   generic_message_types generic_message_handler::get_registered_types() const {
      generic_message_types types;
      for (auto msg : _routers) {
         types.push_back(msg.first);
      }
      return types;
   }
} }
