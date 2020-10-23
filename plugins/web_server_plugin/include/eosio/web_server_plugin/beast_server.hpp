#pragma once

#include <optional>                                   /* optional */            
#include <tuple>                                      /* tuple */

#include <boost/asio/ssl/context.hpp>                 /* ssl::context */

#include <eosio/chain/thread_utils.hpp>               /* named_thread_pool */
#include <eosio/web_server_plugin/http_server_interface.hpp>   /* http_server_generic_interface */
#include <eosio/web_server_plugin/beast_listener.hpp> /* http_listener/https_listener */


namespace eosio::web::beastimpl{

namespace{
   using namespace std;
   using namespace boost::asio;
}

template <typename Child>
struct header_base{
   string_view get(string_view) const {
      return static_cast<Child*>(this)->get(); 
   }
   void set(string_view nm, string_view v) {
       static_cast<Child*>(this)->set(nm, v);
   }
};

template <typename Child>
struct http_server_base {
   void init(server_address&& addr, io_context* ctx) {
       static_cast<Child*>(this)->init(addr, ctx);
   }
   void init(server_address&& addr, uint8_t thread_pool_size) {
       static_cast<Child*>(this)->init(addr, thread_pool_size);
   }
   template <typename Callback>
   void add_handler(string_view path, method_type method, Callback cb) {
       static_cast<Child*>(this)->add_handler(path, forward<Callback>(cb));
   }
   template <typename... Args>
   void run(Args... args) { 
      static_cast<Child*>(this)->run(forward<Args>(args)...);
   }
};

template <typename Child>
struct https_server_base {
    template <typename Callback>
    void init_ssl(string_view cert, string_view pk, Callback&& cb, string_view dh) {
       static_cast<Child*>(this)->init(cert, pk, forward<Callback>(cb), dh);
    }
};

inline io_context* null_context(){
   return NULL;
}

template <typename Listener>
struct http_server_generic : http_server_base<http_server_generic<Listener>>, virtual http_server_interface{
   static constexpr schema_type schema    = schema_type::HTTP;
   using handler_map                      = http_listener::handler_map;
   using http_listener_ptr                = std::shared_ptr<Listener>;

   http_server_generic()
   : running(false),
     context(NULL){
   }

   //<http_server_generic_interface>
   void init(server_address&& addr, io_context* ctx = NULL) override {
      if (ctx){
         context = ctx;
         address = move(addr);
      }
      else {
         init(forward<server_address>(addr), 1);
      }
   }
   void init(server_address&& addr, uint8_t thread_pool_size) override {

      thread_pool.emplace( addr.as_string(), thread_pool_size );
      address = move(addr);
   }

   /**
    * @brief this method is for building an API for the server.
    * Adding handlers permitted only before executing http_server_generic::run method.
    * Reason for this is to gain performance from direct access without 
    * using synchronization primitives while reading from the internal map.
    * @param path http path. server first checks for exact match and if not found
    * it uses string::starts_with so if you specify partial path
    * it will be called for all requests where path starts from this string
    * @param method GET or POST
    * currently there is no support for having GET and POST requests with same path.
    * @param callback callback to fill out client's response. server get's 
    * related data through the callback parameters (method, other headers, etc)
    * @throw other_http_exception if server is already running
    */
   void add_handler(string_view path, method_type method, server_handler callback) override {
      EOS_ASSERT(!running, http_exception, "can't add API after server has started");

      handlers[{string(path), method}] = callback;
   }
   
   template <typename... Args>
   void run(Args... args){
      tcp::endpoint ep;
      if (address.host.empty())
         ep = tcp::endpoint(tcp::v4(), address.port);
      else
         ep = tcp::endpoint(make_address(address.host), address.port);
      listener.reset( new Listener{move(ep), get_executor(), handlers, forward<Args>(args)...} );
      listener->run();
      running = true;
   }

   /**
    * this method starts listening socket and accepting connections 
    */
   void run() override {
      //TODO: make something more reliable
      if constexpr ( is_constructible_v<Listener, tcp::endpoint, io_context&, handler_map> )
         http_server_generic<Listener>::template run();
   }

   void stop() override {
      if (running && listener){
         listener->stop_async();
         running = false;
      }
   }
   //</http_server_generic_interface>

   inline io_context& get_executor() {
      EOS_ASSERT(context || thread_pool, eosio::chain::other_http_exception, "both context and thread_pool not initialized");
      return context ? *context : thread_pool->get_executor();
   }
   inline const server_address& serv_addr() const {
      return address;
   }
   inline const handler_map& callbacks() const {
      return handlers;
   }
   inline bool is_running() const {
      return running;
   }
   inline const Listener* get_listener() const {
      return listener.get();
   }

protected:
   bool                                            running;
   handler_map                                     handlers;
   io_context*                                     context;
   server_address                                  address;
   optional<eosio::chain::named_thread_pool>       thread_pool;
   http_listener_ptr                               listener;
};

using http_server = http_server_generic<http_listener>;

template <typename Listener>
struct https_server_generic : http_server_generic<Listener>, https_server_base<https_server_generic<Listener>>, virtual https_server_interface{
   static constexpr schema_type schema    = schema_type::HTTPS;
   using ssl_context                      = boost::asio::ssl::context;
   using https_listener_ptr               = std::shared_ptr<Listener>;

   https_server_generic() : ssl_ctx(ssl::context::tlsv12_server){}
   virtual ~https_server_generic(){}

   //<http_server_generic_interface>
   void run() override {
      http_server_generic<Listener>::template run<ssl_context&>(ssl_ctx);
   }

   //rest methods are implemented in http_server_generic
   //</http_server_generic_interface>

   //<https_server_interface>
   /**
    * @param cert certificate in PEM format
    * @param pk server's private ket
    * @param pwd_callback password callback. optional
    * @param dh parameters for Ephemeral Diffie-Hellman key agreement protocol. optional
    * if not specified static Diffie-Hellman protocol will be used.
    */
   void init_ssl(std::string_view cert, 
                 std::string_view pk, 
                 password_callback pwd_callback = empty_callback, 
                 std::string_view dh = {}) override {
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

   inline ssl_context& get_ssl_context() {
      return ssl_ctx;
   }
   //</https_server_interface>
protected:
   ssl_context             ssl_ctx;
   https_listener_ptr      ssl_listener;
};

using https_server = https_server_generic<https_listener>;

struct web_server_factory : http_server_generic_factory_interface{
   web_server_factory();
   virtual ~web_server_factory();

   //caller is responsible for deletion of server object
   virtual http_server_interface* create_server(server_address&& address, boost::asio::io_context* context);
};

}