#include <memory>                                     /* unique_ptr */
#include <utility>                                    /* forward */

#include <boost/asio/ip/tcp.hpp>                      /* tcp::endpoint */
#include <boost/asio/ip/address.hpp>                  /* ip::make_address */

#include <eosio/web_server_plugin/beast_server.hpp>   /* http_server/https_server */

using namespace std;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using namespace boost::asio::ip;
using namespace eosio::web;
using namespace eosio::web::beastimpl;
using namespace eosio::chain;

http_server::http_server()
   : running(false),
     context(NULL){
}
http_server::~http_server(){}

void http_server::init(server_address&& addr, boost::asio::io_context* ctx){

   if (ctx){
      context = ctx;
      address = move(addr);
   }
   else {
      init(forward<server_address>(address), 1);
   }
}
void http_server::init(server_address&& addr, uint8_t thread_pool_size){

   thread_pool.emplace( addr.as_string(), thread_pool_size );//TODO customize
   address = move(addr);
}

void http_server::add_handler(std::string_view path, server_handler callback){
   EOS_ASSERT(!running, other_http_exception, "can't add API after server has started");

   handlers[string(path)] = callback;
}

void http_server::run(){
   auto ep = tcp::endpoint(make_address(address.host), address.port);
   listener.emplace(move(ep), get_executor(), handlers);
   listener->run();
}

https_server::https_server() : http_server(), ssl_ctx{ssl::context::tls}{}
https_server::~https_server(){}

void https_server::run(){
   auto ep = tcp::endpoint(make_address(address.host), address.port);
   ssl_listener.emplace(move(ep), get_executor(), handlers, ssl_ctx);
   ssl_listener->run();
}
void https_server::init_ssl(std::string_view cert, std::string_view pk){

}

web_server_factory::web_server_factory(){}
web_server_factory::~web_server_factory(){}

Ihttp_server* web_server_factory::create_server(server_address&& address, boost::asio::io_context* context){
   
   unique_ptr<Ihttp_server> p_srv;
   
   switch (address.schema){
      case schema_type::HTTP:
      p_srv.reset(new http_server());
      break;
      case schema_type::HTTPS:
      p_srv.reset(new https_server());
      break;
      default:
      FC_EXCEPTION(uri_parse_exception, "unknown schema: ${s}", ("s", address.schema));
   }

   p_srv->init(forward<server_address>(address), context);
   return p_srv.release();
}