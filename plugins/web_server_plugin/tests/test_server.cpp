#define BOOST_TEST_MODULE web_server_test
#include <boost/test/included/unit_test.hpp>
#include <fstream>

#include <eosio/web_server_plugin/beast_server.hpp>
#include <test_config.hpp>

using namespace eosio::web;
using namespace eosio::web::beastimpl;
using namespace boost::asio;
using namespace std;

struct http_server_data{
   http_server_data()
   : auth{ schema_type::HTTP, "", 0 }
   {}
   server_address serv_addr() {
      return auth;
   }

   io_context     context;
   server_address auth;
};

struct https_server_data : http_server_data{

   static string file_to_string(string_view path){
      ifstream file(path);
      return string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
   }

   https_server_data()
    : http_server_data(){
       //was generated with:
       //openssl dhparam -out dhparams.pem 2048
       dh = R"=====(
-----BEGIN DH PARAMETERS-----
MIIBCAKCAQEA0Zy20tn+KDZDEgaANfnyP+XF0NFx7TErorogHVnZCZHqVbs8rSde
efV5m8CmeNk/xafvE9R524tTZGQBEZsvQJanGkp4SiDfuSJe+zDjHXgU3sHf4xV0
lOLgqHm2gjKmiRE+OdeVGYNKpEew9lX+Gf8Y11Ii9c0Lf0YRFodqKq7ytJ1/iHum
cmjrogPqQBs7G8l8PHdKPng4W+WxoH6a9DeebAJEZ1VZhNOzLgq4tic4pj/ujP4J
ahMEX81pBVYukc/5eOvosnrY0UcPD8pxOpVYiV9Ei0SIqfGcAW6G5h2kvevXK6k5
zLA28uWq1LB9Syg6ZG5ZmRf1VOai1lzOiwIBAg==
-----END DH PARAMETERS-----
)=====";

      //those keys generated during compilation. See cmake file.
      cert = file_to_string(test::keypath + "/test_ecdsa.crt");
      pk = file_to_string(test::keypath + "/test_ecdsa.pem");
    }
   
   string         cert;
   string         pk;
   string         dh;

};

struct mock_http_listener : http_listener{
   bool is_running;

   mock_http_listener(const http_listener::endpoint& ep, 
                      http_listener::io_context& ctx, 
                      const http_listener::handler_map& callbacks)
      : http_listener(ep, ctx, callbacks), is_running(false){}
   
   void run(){
      is_running = true;
   }

   void stop_async(){
      is_running = false;
   }
};

struct mock_https_listener : https_listener{
   bool is_running;

   mock_https_listener(const http_listener::endpoint& ep, 
                       http_listener::io_context& ctx, 
                       const http_listener::handler_map& callbacks,
                       https_listener::ssl_context& ssl_ctx)
      : https_listener(ep, ctx, callbacks, ssl_ctx), is_running(false){}
   
   void run(){
      is_running = true;
   }

   void stop_async(){
      is_running = false;
   }
};

//boost requirement to have it under std
namespace std{
template <typename Stream>
inline Stream& operator << (Stream& o, method_type method){
   switch (method){
      case method_type::GET:
      o << "GET";
      break;
      case method_type::POST:
      o << "POST";
      break;
      default:
      o << "other(" << static_cast<underlying_type_t<method_type>>(method) << ")";
   }
   return o;
}

template <typename Stream>
inline Stream& operator << (Stream& o, const server_path& p){
   return o << p.path << ":" << p.method;
}

template <typename Stream>
inline Stream& operator << (Stream& o, const server_address& s){
   return o << s.as_string();
}

}//anonymous namespace

BOOST_AUTO_TEST_SUITE(http_test_server)

   BOOST_FIXTURE_TEST_CASE(init_with_context, http_server_data){

      http_server srv;
      srv.init(serv_addr(), &context);

      BOOST_TEST(srv.serv_addr() == auth);
      BOOST_TEST(&srv.get_executor() == &context);
      BOOST_TEST(srv.is_running() == false);
   }

   BOOST_FIXTURE_TEST_CASE(init_with_1_thread, http_server_data){

      http_server srv;
      srv.init(serv_addr());

      BOOST_TEST(srv.serv_addr() == auth);
      BOOST_TEST(&srv.get_executor() != null_context());
      BOOST_TEST(srv.is_running() == false);
   }

   BOOST_FIXTURE_TEST_CASE(init_with_4_threads, http_server_data){

      http_server srv;
      srv.init(serv_addr(), 4);

      BOOST_TEST(srv.serv_addr() == auth);
      BOOST_TEST(&srv.get_executor() != null_context());
      BOOST_TEST(srv.is_running() == false);
   }

   BOOST_FIXTURE_TEST_CASE(add_handlers, http_server_data){

      http_server srv;
      srv.init(serv_addr());
      srv.add_handler("test", method_type::GET, server_handler{});
      BOOST_TEST(srv.callbacks().size() == 1);
      server_path serv_name = {"test", method_type::GET};
      BOOST_TEST(srv.callbacks().begin()->first == serv_name);
   }

   BOOST_FIXTURE_TEST_CASE(run_stop, http_server_data){

      http_server_generic<mock_http_listener> srv;
      srv.init(serv_addr());
      srv.add_handler("test", method_type::GET, server_handler{});
      BOOST_TEST(!srv.is_running());
      BOOST_TEST(!srv.get_listener());
      srv.run();
      BOOST_TEST(srv.is_running());
      BOOST_TEST(srv.get_listener());
      BOOST_TEST(srv.get_listener()->is_running);
      srv.stop();
      BOOST_TEST(!srv.is_running());
      BOOST_TEST(!srv.get_listener()->is_running);
   }

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(https_test_server)

   BOOST_FIXTURE_TEST_CASE(init_ssl, https_server_data){

      https_server srv;
      srv.init_ssl(cert, pk);

      BOOST_TEST(srv.get_ssl_context().native_handle());
   }
   BOOST_FIXTURE_TEST_CASE(init_ssl_w_pwd_callback, https_server_data){

      https_server srv;
      srv.init_ssl(cert, pk, empty_callback);

      BOOST_TEST(srv.get_ssl_context().native_handle());
   }
   BOOST_FIXTURE_TEST_CASE(init_ssl_w_pwd_callback_dh, https_server_data){

      https_server srv;
      srv.init_ssl(cert, pk, empty_callback, dh);

      BOOST_TEST(srv.get_ssl_context().native_handle());
   }

   BOOST_FIXTURE_TEST_CASE(run_stop, https_server_data){

      https_server_generic<mock_https_listener> srv;
      srv.init(serv_addr());
      srv.add_handler("test", method_type::GET, server_handler{});
      BOOST_TEST(!srv.is_running());
      BOOST_TEST(!srv.get_listener());
      srv.run();
      BOOST_TEST(srv.is_running());
      BOOST_TEST(srv.get_listener());
      BOOST_TEST(srv.get_listener()->is_running);
      srv.stop();
      BOOST_TEST(!srv.is_running());
      BOOST_TEST(!srv.get_listener()->is_running);
   }

BOOST_AUTO_TEST_SUITE_END()