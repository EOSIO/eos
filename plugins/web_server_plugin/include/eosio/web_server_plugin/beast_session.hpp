#pragma once

#include <boost/asio/ssl/context.hpp>                 /* ssl::context */
#include <boost/asio/io_context.hpp>                  /* io_context */

namespace eosio::web{

struct http_session{
   using handler_map =        std::unordered_map<std::string, server_handler>;
   using io_context =         boost::asio::io_context;
   using ssl_context =        boost::asio::ssl::context;

   ssl_context&                        ssl_ctx;
   io_context&                         execution_ctx;
   const handler_map&                  handlers;
};

}