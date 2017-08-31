#include <eos/http_plugin/http_plugin.hpp>

#include <fc/network/ip.hpp>
#include <fc/log/logger_config.hpp>

#include <boost/asio.hpp>
#include <boost/optional.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/logger/stub.hpp>

#include <thread>
#include <memory>

namespace eos {
   namespace asio = boost::asio;

   using std::map;
   using std::string;
   using boost::optional;
   using boost::asio::ip::tcp;
   using std::shared_ptr;
   using websocketpp::connection_hdl;


   namespace detail {

      struct asio_with_stub_log : public websocketpp::config::asio {
          typedef asio_with_stub_log type;
          typedef asio base;

          typedef base::concurrency_type concurrency_type;

          typedef base::request_type request_type;
          typedef base::response_type response_type;

          typedef base::message_type message_type;
          typedef base::con_msg_manager_type con_msg_manager_type;
          typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

          /// Custom Logging policies
          /*typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::elevel> elog_type;
          typedef websocketpp::log::syslog<concurrency_type,
              websocketpp::log::alevel> alog_type;
          */
          //typedef base::alog_type alog_type;
          //typedef base::elog_type elog_type;
          typedef websocketpp::log::stub elog_type;
          typedef websocketpp::log::stub alog_type;

          typedef base::rng_type rng_type;

          struct transport_config : public base::transport_config {
              typedef type::concurrency_type concurrency_type;
              typedef type::alog_type alog_type;
              typedef type::elog_type elog_type;
              typedef type::request_type request_type;
              typedef type::response_type response_type;
              typedef websocketpp::transport::asio::basic_socket::endpoint
                  socket_type;
          };

          typedef websocketpp::transport::asio::endpoint<transport_config>
              transport_type;

          static const long timeout_open_handshake = 0;
      };
   }

   using websocket_server_type = websocketpp::server<detail::asio_with_stub_log>;

   class http_plugin_impl {
      public:
         //shared_ptr<std::thread>  http_thread;
         //asio::io_service         http_ios;
         map<string,url_handler>  url_handlers;
         optional<tcp::endpoint>  listen_endpoint;

         websocket_server_type    server;
   };

   http_plugin::http_plugin():my(new http_plugin_impl()){}
   http_plugin::~http_plugin(){}

   void http_plugin::set_program_options(options_description&, options_description& cfg) {
      cfg.add_options()
            ("http-server-endpoint", bpo::value<string>()->default_value("127.0.0.1:8888"),
             "The local IP and port to listen for incoming http connections.")
            ;
   }

   void http_plugin::plugin_initialize(const variables_map& options) {
      if(options.count("http-server-endpoint")) {
        #if 0
         auto lipstr = options.at("http-server-endpoint").as< string >();
         auto fcep = fc::ip::endpoint::from_string(lipstr);
         my->listen_endpoint = tcp::endpoint(boost::asio::ip::address_v4::from_string((string)fcep.get_address()), fcep.port());
        #endif
         auto resolver = std::make_shared<tcp::resolver>( std::ref( app().get_io_service() ) );
         if( options.count( "http-server-endpoint" ) ) {
           auto lipstr =  options.at("http-server-endpoint").as< string >();
           auto host = lipstr.substr( 0, lipstr.find(':') );
           auto port = lipstr.substr( host.size()+1, lipstr.size() );
           idump((host)(port));
           tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
           my->listen_endpoint = *resolver->resolve( query);
           ilog("configured http to listen on ${h}:${p}", ("h",host)("p",port));
         }

         // uint32_t addr = my->listen_endpoint->address().to_v4().to_ulong();
         // auto fcep = fc::ip::endpoint (addr,my->listen_endpoint->port());
      }
   }

   void http_plugin::plugin_startup() {
      if(my->listen_endpoint) {

         //my->http_thread = std::make_shared<std::thread>([&](){
            ilog("start processing http thread");
            try {
               my->server.clear_access_channels(websocketpp::log::alevel::all);
               my->server.init_asio(&app().get_io_service()); //&my->http_ios);
               my->server.set_reuse_addr(true);

               my->server.set_http_handler([&](connection_hdl hdl) {
                  auto con = my->server.get_con_from_hdl(hdl);
                  try {
                     //ilog("handle http request: ${url}", ("url",con->get_uri()->str()));
                     //ilog("${body}", ("body", con->get_request_body()));

                     auto body = con->get_request_body();
                     auto resource = con->get_uri()->get_resource();
                     auto handler_itr = my->url_handlers.find(resource);
                     if(handler_itr != my->url_handlers.end()) {
                        handler_itr->second(resource, body, [con,this](int code, string body) {
                           con->set_body(body);
                           con->set_status(websocketpp::http::status_code::value(code));
                        });
                     } else {
                        wlog("404 - not found: ${ep}", ("ep",resource));
                        con->set_body("Unknown Endpoint");
                        con->set_status(websocketpp::http::status_code::not_found);
                     }
                  } catch( const fc::exception& e ) {
                     elog( "http: ${e}", ("e",e.to_detail_string()));
                        con->set_body(e.to_detail_string());
                        con->set_status(websocketpp::http::status_code::internal_server_error);
                  } catch( const std::exception& e ) {
                     elog( "http: ${e}", ("e",e.what()));
                        con->set_body(e.what());
                        con->set_status(websocketpp::http::status_code::internal_server_error);
                  } catch( ... ) {
                        con->set_body("unknown exception");
                        con->set_status(websocketpp::http::status_code::internal_server_error);
                  }
               });

               ilog("start listening for http requests");
               my->server.listen(*my->listen_endpoint);
               my->server.start_accept();

           //    my->http_ios.run();
               ilog("http io service exit");
            } catch ( const fc::exception& e ){
               elog( "http: ${e}", ("e",e.to_detail_string()));
            } catch ( const std::exception& e ){
               elog( "http: ${e}", ("e",e.what()));
            } catch (...) {
                elog("error thrown from http io service");
            }
         //});

      }
   }

   void http_plugin::plugin_shutdown() {
     // if(my->http_thread) {
         if(my->server.is_listening())
             my->server.stop_listening();
     //    my->http_ios.stop();
     //    my->http_thread->join();
     //    my->http_thread.reset();
     // }
   }

   void http_plugin::add_handler(const string& url, const url_handler& handler) {
      ilog( "add api url: ${c}", ("c",url) );
      app().get_io_service().post([=](){
        my->url_handlers.insert(std::make_pair(url,handler));
      });
   }
}
