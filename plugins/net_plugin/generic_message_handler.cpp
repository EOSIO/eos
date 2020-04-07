#include <eosio/chain/types.hpp>

#include <eosio/net_plugin/generic_message_handler.hpp>

namespace eosio { namespace net {

   generic_message_handler::generic_router::generic_router(const std::type_info& type)
      : type(type) {
   }

   void generic_message_handler::route(const generic_message& msg) {
      auto existing_router = _routers.find(msg.type);
      if (existing_router == _routers.end()) {
         dlog( "No routes for generic_message of type: ${type}", ("type", msg.type) );
         return;
      }

      existing_router->second->route(msg);
   }
} }
