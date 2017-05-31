#pragma once
#include <fc/rpc/api_connection.hpp>
#include <fc/rpc/state.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/variant.hpp>

namespace fc { namespace rpc {

   class websocket_api_connection : public api_connection
   {
      public:
         websocket_api_connection( fc::http::websocket_connection& c );
         ~websocket_api_connection();

         virtual variant send_call(
            api_id_type api_id,
            string method_name,
            variants args = variants() ) override;
         virtual variant send_callback(
            uint64_t callback_id,
            variants args = variants() ) override;
         virtual void send_notice(
            uint64_t callback_id,
            variants args = variants() ) override;

      protected:
         std::string on_message(
            const std::string& message,
            bool send_message = true );

         fc::http::websocket_connection&  _connection;
         fc::rpc::state                   _rpc_state;
   };

} } // namespace fc::rpc
