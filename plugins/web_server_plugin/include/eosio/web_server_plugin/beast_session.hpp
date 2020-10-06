#pragma once

#include <variant>                                    /* variant */

#include <boost/asio/ssl/context.hpp>                 /* ssl::context */
#include <boost/asio/io_context.hpp>                  /* io_context */
#include <boost/beast/core/tcp_stream.hpp>            /* tcp_stream */
#include <boost/beast/core/flat_buffer.hpp>           /* flat_buffer */
#include <boost/beast/http/empty_body.hpp>            /* empty_body */
#include <boost/beast/http/string_body.hpp>           /* string_body */
#include <boost/beast/http/message.hpp>               /* request, response */

namespace eosio::web{

struct http_session{
   using handler_map =        std::unordered_map<std::string, server_handler>;
   using io_context  =        boost::asio::io_context;
   using ssl_context =        boost::asio::ssl::context;
   using stream_type =        boost::beast::tcp_stream;
   using flat_buffer =        boost::beast::flat_buffer;
   using empty_body  =        boost::beast::http::empty_body;
   using string_body =        boost::beast::http::string_body;
   using get_request =        boost::beast::http::request<empty_body>;
   using post_request=        boost::beast::http::request<string_body>;
   using var_request =        std::variant<get_request, post_request>;

   stream_type                         stream;
   flat_buffer                         additional_buffer;
   var_request                         client_request;
   ssl_context&                        ssl_ctx;
   io_context&                         execution_ctx;
   const handler_map&                  handlers;
};

}