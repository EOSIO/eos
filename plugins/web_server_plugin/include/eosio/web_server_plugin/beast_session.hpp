#pragma once

#include <variant>                                    /* variant */
#include <unordered_map>                              /* unordered_map */

#include <boost/asio/ssl/context.hpp>                 /* ssl::context */
#include <boost/asio/io_context.hpp>                  /* io_context */
#include <boost/beast/core/tcp_stream.hpp>            /* tcp_stream */
#include <boost/beast/core/flat_buffer.hpp>           /* flat_buffer */
#include <boost/beast/http/empty_body.hpp>            /* empty_body */
#include <boost/beast/http/string_body.hpp>           /* string_body */
#include <boost/beast/http/message.hpp>               /* request, response */
#include <boost/beast/ssl/ssl_stream.hpp>             /* ssl_stream */
#include <boost/asio/dispatch.hpp>                    /* dispatch */
#ifdef DEBUG
#include <boost/filesystem/fstream.hpp>               /* ofstream */
#endif

#include <eosio/web_server_plugin/http_server_interface.hpp>

namespace eosio::web::beastimpl{

namespace {
   using namespace std;
}

template<typename Child>
struct http_session_base{

   void run() {
      static_cast<Child*>(this)->run();
   }
};

struct server_path {
   string path;
   method_type method;

   inline bool operator == (const server_path& other) const{
      return tie(path, method) == tie(other.path, other.method);
   }
};

struct server_path_hash{
   size_t operator()(const server_path& k) const {
      using underlying_method_type = underlying_type_t<method_type>;
      return hash<string>()(k.path) ^ (hash<underlying_method_type>()(static_cast<underlying_method_type>(k.method)) << 1);
   }
};

//TODO:
//add shutdown boolean + close method. same for listener (stop accepting new)
template<typename Stream>
struct http_session_generic : http_session_base<http_session_generic<Stream>>{
   using handler_map =        std::unordered_map<server_path, server_handler, server_path_hash>;
   using io_context  =        boost::asio::io_context;
   using ssl_context =        boost::asio::ssl::context;
   using socket      =        boost::asio::ip::tcp::socket;
   using flat_buffer =        boost::beast::flat_buffer;
   using empty_body  =        boost::beast::http::empty_body;
   using string_body =        boost::beast::http::string_body;
   using get_request =        boost::beast::http::request<empty_body>;
   using post_request=        boost::beast::http::request<string_body>;
   using var_request =        std::variant<get_request, post_request>;

   const handler_map&                  handlers;
   Stream                              stream;
   flat_buffer                         additional_buffer;
   var_request                         client_request;

   /**
    * @brief constructor for http stream 
    */
   template <typename = std::enable_if<std::is_same_v<Stream, boost::beast::tcp_stream>>>
   http_session_generic(const handler_map& callbacks, socket&& s)
    : handlers(callbacks), stream(std::move(s)){}

   /**
    * @brief constructor for https stream 
    */
   template <typename = std::enable_if<std::is_same_v<Stream, 
                                                      boost::beast::ssl_stream<boost::beast::tcp_stream>>>>
   http_session_generic(const handler_map& callbacks, socket&& s, ssl_context& ssl_context)
    : handlers(callbacks), stream(std::move(s), ssl_context){}

   void run(){
      boost::asio::dispatch(stream.get_executor(), [](){});
   }
};

using http_session = http_session_generic<boost::beast::tcp_stream>;

struct https_session : http_session_generic<boost::beast::ssl_stream<boost::beast::tcp_stream>>{
   using base        =        http_session_generic<boost::beast::ssl_stream<boost::beast::tcp_stream>>;
   using ssl_context =        base::ssl_context;
   using handler_map =        base::handler_map;
   using socket      =        base::socket;

   ssl_context&                        ssl_ctx;

#ifdef DEBUG
   /* TODO: add dumping keys for debug */
   std::ofstream              session_key_log;
#endif

   https_session(const handler_map& callbacks, socket&& s, ssl_context& ssl_context)
    : base(callbacks, std::forward<socket>(s), ssl_context), ssl_ctx(ssl_context){}
};

}