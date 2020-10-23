#include <memory>                                     /* unique_ptr */
#include <utility>                                    /* forward */

#include <boost/asio/ip/tcp.hpp>                      /* tcp::endpoint */
#include <boost/asio/ip/address.hpp>                  /* ip::make_address */

#include <eosio/web_server_plugin/beast_server.hpp>   /* http_server_generic/https_server */

using namespace std;
namespace asio = boost::asio;
namespace ssl = asio::ssl;
using namespace boost::asio::ip;
using namespace eosio::web;
using namespace eosio::web::beastimpl;
using namespace eosio::chain;

/* 
void http_server_generic::init(server_address&& addr, boost::asio::io_context* ctx){

   if (ctx){
      context = ctx;
      address = move(addr);
   }
   else {
      init(forward<server_address>(address), 1);
   }
}
void http_server_generic::init(server_address&& addr, uint8_t thread_pool_size){

   thread_pool.emplace( addr.as_string(), thread_pool_size );
   address = move(addr);
}

void http_server_generic::add_handler(std::string_view path, method_type method, server_handler callback){
   EOS_ASSERT(!running, http_exception, "can't add API after server has started");

   handlers[string(path)] = callback;
}

void http_server_generic::run(){
   auto ep = tcp::endpoint(make_address(address.host), address.port);
   listener.reset( new http_listener{move(ep), get_executor(), handlers} );
   listener->run();
}

void http_server_generic::stop(){
   if (listener)
      listener->stop_async();
}

https_server::https_server() : http_server_generic(), ssl_ctx{ssl::context::tls}{}
https_server::~https_server(){}

void https_server::run(){
   auto ep = tcp::endpoint(make_address(address.host), address.port);
   listener.reset( new https_listener{move(ep), get_executor(), handlers, ssl_ctx} );
   listener->run();
}

void https_server::init_ssl(std::string_view cert, std::string_view pk, password_callback pwd_callback, std::string_view dh){
   ssl_ctx.set_password_callback(pwd_callback);
   ssl_ctx.set_options(ssl::context::default_workarounds |
                       ssl::context::no_sslv2 |
                       ssl::context::no_sslv3 |
                       ssl::context::no_tlsv1 |
                       ssl::context::no_tlsv1_1 |
                       ssl::context::single_dh_use);
   ssl_ctx.use_certificate_chain(asio::buffer(cert));
   ssl_ctx.use_private_key(asio::buffer(pk), ssl::context::file_format::pem);
   
   if (!dh.empty())
      ssl_ctx.use_tmp_dh(asio::buffer(dh));

   //just in case you find missing elliptic curve setup here:
   //in openssl version > 1.1.0 elliptic curve is enabled by default. no need to set it explicitly
   //Also I don't see any reason to specify curve type manually. ssl maintains most releavant curves by itself:
   //The Change Log of 1.1.0 states:
   //Change the ECC default curve list to be this, in order: x25519, secp256r1, secp521r1, secp384r1.
   
   //EECDH key agreement will be used only if dh parameter was set 
   const char* cipher_list = "EECDH+ECDSA+AESGCM:EECDH+aRSA+AESGCM:EECDH+ECDSA+SHA384:EECDH+ECDSA+SHA256:AES256:"
                             "!DHE:!RSA:!AES128:!RC4:!DES:!3DES:!DSS:!SRP:!PSK:!EXP:!MD5:!LOW:!aNULL:!eNULL";
   if(!SSL_CTX_set_cipher_list(ssl_ctx.native_handle(), cipher_list))
      EOS_THROW(chain::http_exception, "Failed to set HTTPS cipher list");
}
*/
web_server_factory::web_server_factory(){}
web_server_factory::~web_server_factory(){}

http_server_interface* web_server_factory::create_server(server_address&& address, boost::asio::io_context* context){
   
   unique_ptr<http_server_interface> p_srv;
   
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