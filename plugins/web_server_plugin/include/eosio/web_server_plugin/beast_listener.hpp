#pragma once

#include <functional>                                    /* reference_wrapper */

#include <boost/asio/io_context.hpp>                     /* io_context */
#include <boost/asio/ip/tcp.hpp>                         /* acceptor */
#include <boost/asio/ssl/context.hpp>                    /* ssl::context */
#include <boost/beast/core/error.hpp>                    /* error_code */

#include <eosio/web_server_plugin/beast_session.hpp>     /* http_session/https_session */

namespace eosio::web::beastimpl{

struct http_listener : std::enable_shared_from_this<http_listener>{
   using socket_acceptor =    boost::asio::ip::tcp::acceptor;
   using io_context =         boost::asio::io_context;
   using io_context_ref =     std::reference_wrapper<io_context>;
   using handler_map =        http_session::handler_map;
   using handler_map_cref =   std::reference_wrapper<const handler_map>;
   using endpoint =           boost::asio::ip::tcp::endpoint;
   using socket =             boost::asio::ip::tcp::socket;
   using accept_handler =     std::function<void(boost::beast::error_code, socket)>;

   endpoint                   address;
   io_context_ref             context;
   socket_acceptor            acceptor;
   handler_map_cref           handlers;

   http_listener(const endpoint auth, io_context& context, const handler_map& handlers);
   void run();

protected:
   void listen();
   void start_accept(accept_handler acc_handler);
   void start_accept();
   void on_accept(boost::beast::error_code ec, socket s);
};

struct https_listener : http_listener, std::enable_shared_from_this<https_listener>{
   using socket_acceptor   = http_listener::socket_acceptor;
   using io_context        = http_listener::io_context;
   using ssl_context       = boost::asio::ssl::context;
   using ssl_context_ref   = std::reference_wrapper<ssl_context>;
   using endpoint          = boost::asio::ip::tcp::endpoint;

   ssl_context_ref            ssl_ctx;

   https_listener(const endpoint& ep, 
                  io_context& ctx, 
                  const handler_map& handlers, 
                  ssl_context& ssl_ctx);
   void start_accept();
   void on_accept(boost::beast::error_code ec, socket s);
};

}//namespace eosio::web::beastimpl