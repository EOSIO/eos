#pragma once
#include <fc/io/json.hpp>
#include <fc/network/http/connection.hpp>
#include <fc/network/http/server.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/rpc/api_connection.hpp>
#include <fc/rpc/state.hpp>

namespace fc { namespace rpc {

   class http_api_connection : public api_connection
   {
      public:
         http_api_connection();
         ~http_api_connection();

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

         void on_request(
            const fc::http::request& req,
            const fc::http::server::response& resp );

         fc::rpc::state                   _rpc_state;
   };

} } // namespace fc::rpc
