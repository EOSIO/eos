#pragma once
#include <eosio/net_plugin/protocol.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/signals2/signal.hpp>
#include <memory>
#include <typeinfo>
#include <unordered_map>
#include "protocol.hpp"

namespace eosio {
   namespace net {
      using boost::signals2::scoped_connection;
      using boost::signals2::signal;

      template<typename T>
      void convert(const bytes& payload, T& t);

      template<typename T>
      void convert(const T& t, bytes& payload);

      class generic_message_handler {
      public:
         struct generic_router;
         using generic_router_ptr = std::shared_ptr<generic_router>;

         struct generic_router {
            generic_router(const std::type_info& type);
            virtual ~generic_router() {}
            virtual void route(const generic_message& msg, const std::string& endpoint) const = 0;
            const std::type_info& type;
         };

         template<typename T>
         struct router : public generic_router {
            typedef void (forward_message)(const T&, const std::string&);

            router();
            ~router() override {}
            void route(const generic_message& msg, const std::string& endpoint) const override;
            signal<forward_message> forward_msg;
         };

         // register a generic_message and function to forward it to when received,
         // should only be called during initialization
         template<typename T, typename ForwardMessage>
         scoped_connection register_msg(ForwardMessage forward_msg);

         // route a generic_message to the functions that were registered for it
         void route(const generic_message& msg, const string& endpoint) const;

         // report all registered generic_messages
         generic_message_types get_registered_types() const;
      private:
         using router_map = std::unordered_map<generic_message_type, generic_router_ptr>;
         router_map _routers;
      };

      template<typename T>
      generic_message_handler::router<T>::router()
      : generic_router{typeid(T)} {
      }

      template<typename T>
      void generic_message_handler::router<T>::route(const generic_message& msg, const string& endpoint) const {
         T t;
         //ilog( "TEST route convert" );
         convert( msg.payload, t );
         //ilog( "TEST route forward_msg: ${f}", ("f", (uint64_t) &forward_msg));
         forward_msg( t, endpoint );
         //ilog( "TEST route done" );
      }

      template<typename T, typename ForwardMessage>
      scoped_connection generic_message_handler::register_msg(ForwardMessage forward_msg) {
         typedef router<T> router_t;
         const std::type_info& type = typeid(T);
         auto existing_router = _routers.find(type.hash_code());
         router_t* msg_router = nullptr;
         if (existing_router != _routers.end()) {
            dlog("register existing type: ${t} (${n}", ("t", type.hash_code())("n", type.name()));
            // type is already registered
            msg_router = dynamic_cast<router_t* >(existing_router->second.get());
            if (msg_router == nullptr) {
               const string error_msg = "Found existing router for type hash code: " +
                  std::to_string(type.hash_code()) + ", but expecting the found router to be a router<" + type.name() +
                  "> instead it was a router<" + existing_router->second->type.name() + ">";
               throw std::runtime_error(error_msg);
            }
         }
         else {
            dlog("register new type: ${t} (${n}", ("t", type.hash_code())("n", type.name()));
            auto actual_router = std::make_shared<router_t>();
            msg_router = actual_router.get();
            generic_router_ptr g_router(actual_router);
            _routers.insert({type.hash_code(), g_router});
         }
         scoped_connection scoped_con = msg_router->forward_msg.connect(forward_msg);
         return scoped_con;
      }
   }
}