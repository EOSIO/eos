#pragma once

#include <functional>                                    /* reference_wrapper */
#include <utility>                                       /* declval */
#include <type_traits>                                   /* tuple */

#include <boost/asio/io_context.hpp>                     /* io_context */
#include <boost/asio/ip/tcp.hpp>                         /* acceptor */
#include <boost/asio/ssl/context.hpp>                    /* ssl::context */
#include <boost/asio/strand.hpp>                         /* strand */
#include <boost/beast/core/error.hpp>                    /* error_code */

#include <eosio/web_server_plugin/beast_session.hpp>     /* http_session/https_session */
#include <eosio/web_server_plugin/global.hpp>            /* logger */

namespace eosio::web::beastimpl{

#define HANDLE_SHUTDOWN(socket)\
if (shutdown){\
   fc_ilog(logger, "shutdown command was sent, closing connection");\
   beast::error_code socket_ec;\
   socket.close(socket_ec);\
   if (socket_ec){\
      fc_elog(logger, "socket.close error: {$ec}", ("ec", socket_ec.message()));\
   }\
   return;\
}

namespace{
   using namespace eosio::chain;
   using namespace eosio::web;
   using namespace eosio::web::beastimpl;
   namespace asio = boost::asio;
   namespace beast = boost::beast;
   using namespace asio::ip;
   using namespace std;
}

template <typename Child>
struct http_listener_base {
   void run() {
      static_cast<Child*>(this)->run();
   }
   void stop_async(){
      static_cast<Child*>(this)->stop_async();
   }
   void stop(){
      static_cast<Child*>(this)->stop();
   }
   /*template <typename Socket>
   void on_accept(boost::beast::error_code ec, Socket s){
      static_cast<Child*>(this)->on_accept(ec, forward<Socket>(s));
   }*/
};

struct http_listener : http_listener_base<http_listener>, enable_shared_from_this<http_listener>{
   using socket_acceptor    = boost::asio::ip::tcp::acceptor;
   using io_context         = boost::asio::io_context;
   using io_context_ref     = std::reference_wrapper<io_context>;
   using strand             = boost::asio::strand<io_context::executor_type>;
   using handler_map        = http_session::handler_map;
   using handler_map_cref   = std::reference_wrapper<const handler_map>;
   using endpoint           = boost::asio::ip::tcp::endpoint;
   using socket             = boost::asio::ip::tcp::socket;
   using accept_handler     = std::function<void(boost::beast::error_code, socket)>;

   endpoint                   address;
   io_context_ref             context;
   socket_acceptor            acceptor;
   handler_map_cref           handlers;

   http_listener(const endpoint& ep, 
                 io_context& ctx, 
                 const handler_map& callbacks)
   : address(ep), 
     context(ctx),
     acceptor(asio::make_strand(ctx)), 
     handlers(callbacks),
     shutdown(false),
     this_strand(ctx.get_executor()){
   }
   
   ~http_listener(){
      //TODO: add shutdown logic here
      //if (!shutdown)
   }

   void run(){
      listen();
      start_accept();
   }
   void stop_async(){
      auto pthis = shared_from_this();
      asio::post(this_strand, [pthis](){ pthis->stop(); });
   }

protected:
   bool                       shutdown;
   strand                     this_strand;

   void listen(){
   
      beast::error_code ec;

      acceptor.open(address.protocol(), ec);
      EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.open failed with error: ${ec}", ("ec", ec.message()));

      // Allow address reuse
      //this is to avoid 'address in use' error
      //explanation and how it works for different platforms you can read there:
      //https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ
      acceptor.set_option(asio::socket_base::reuse_address(true), ec);
      EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.set_option failed with error: ${ec}", ("ec", ec.message()));
      
      acceptor.bind(address, ec);
      EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.bind failed with error: ${ec}", ("ec", ec.message()));

      acceptor.listen(asio::socket_base::max_listen_connections, ec);
      EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.listen failed with error: ${ec}", ("ec", ec.message()));
   }

   void start_accept(accept_handler acc_handler){
      acceptor.async_accept(this_strand, acc_handler);
   }

   void stop(){
      if (acceptor.is_open()){
         beast::error_code ec;
         
         acceptor.close(ec);
         EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.close failed with error: {ec}", ("ec", ec.message()));
      }
      shutdown = true;
   }

   void start_accept(){
      auto pthis = shared_from_this();
      start_accept([pthis](beast::error_code ec, tcp::socket s){
                  pthis->on_accept(ec, move(s));
                });
   }

protected:
   void on_accept(boost::beast::error_code ec, socket s){
      try{
         EOS_ASSERT(!ec, tcp_accept_exception, "on_accept got error: ${ec}", ("ec", ec.message()));
         HANDLE_SHUTDOWN(s);


         //create session here and return pointer to caller
      }
      catch (const tcp_accept_exception& e){
         fc_elog(logger, "tcp_accept_exception: ${e}", ("e", e));
      }
   }

   //TODO: create contaner for managing sessions
};

struct https_listener : http_listener, enable_shared_from_this<https_listener>{
   using ssl_context       = boost::asio::ssl::context;
   using ssl_context_ref   = std::reference_wrapper<ssl_context>;

   ssl_context_ref            ssl_ctx;

   https_listener(const endpoint& ep, 
                 io_context& ctx, 
                 const handler_map& callbacks,
                 ssl_context& ssl_cont)
   : http_listener(ep, ctx, callbacks), ssl_ctx(ssl_cont){}

};

}//namespace eosio::web::beastimpl