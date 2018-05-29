/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/network/ip.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/openssl.hpp>

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

namespace eosio {

   static appbase::abstract_plugin& _http_plugin = app().register_plugin<http_plugin>();

   namespace asio = boost::asio;

   using std::map;
   using std::string;
   using boost::optional;
   using boost::asio::ip::tcp;
   using std::shared_ptr;
   using websocketpp::connection_hdl;


   namespace detail {

      template<class T>
      struct asio_with_stub_log : public websocketpp::config::asio {
          typedef asio_with_stub_log type;
          typedef asio base;

          typedef base::concurrency_type concurrency_type;

          typedef base::request_type request_type;
          typedef base::response_type response_type;

          typedef base::message_type message_type;
          typedef base::con_msg_manager_type con_msg_manager_type;
          typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;

          typedef websocketpp::log::stub elog_type;
          typedef websocketpp::log::stub alog_type;

          typedef base::rng_type rng_type;

          struct transport_config : public base::transport_config {
              typedef type::concurrency_type concurrency_type;
              typedef type::alog_type alog_type;
              typedef type::elog_type elog_type;
              typedef type::request_type request_type;
              typedef type::response_type response_type;
              typedef T socket_type;
          };

          typedef websocketpp::transport::asio::endpoint<transport_config>
              transport_type;

          static const long timeout_open_handshake = 0;
      };
   }

   using websocket_server_type = websocketpp::server<detail::asio_with_stub_log<websocketpp::transport::asio::basic_socket::endpoint>>;
   using websocket_server_tls_type =  websocketpp::server<detail::asio_with_stub_log<websocketpp::transport::asio::tls_socket::endpoint>>;
   using ssl_context_ptr =  websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;

   class http_plugin_impl {
      public:
         map<string,url_handler>  url_handlers;
         optional<tcp::endpoint>  listen_endpoint;
         string                   access_control_allow_origin;
         string                   access_control_allow_headers;
         string                   access_control_max_age;
         bool                     access_control_allow_credentials = false;

         websocket_server_type    server;

         optional<tcp::endpoint>  https_listen_endpoint;
         string                   https_cert_chain;
         string                   https_key;

         websocket_server_tls_type https_server;

         ssl_context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
            ssl_context_ptr ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(asio::ssl::context::sslv23_server);

            try {
               ctx->set_options(asio::ssl::context::default_workarounds |
                                asio::ssl::context::no_sslv2 |
                                asio::ssl::context::no_sslv3 |
                                asio::ssl::context::no_tlsv1 |
                                asio::ssl::context::no_tlsv1_1 |
                                asio::ssl::context::single_dh_use);

               ctx->use_certificate_chain_file(https_cert_chain);
               ctx->use_private_key_file(https_key, asio::ssl::context::pem);

               //going for the A+! Do a few more things on the native context to get ECDH in use

               fc::ec_key ecdh = EC_KEY_new_by_curve_name(NID_secp384r1);
               if (!ecdh)
                  FC_THROW("Failed to set NID_secp384r1");
               if(SSL_CTX_set_tmp_ecdh(ctx->native_handle(), (EC_KEY*)ecdh) != 1)
                  FC_THROW("Failed to set ECDH PFS");

               if(SSL_CTX_set_cipher_list(ctx->native_handle(), \
                  "EECDH+ECDSA+AESGCM:EECDH+aRSA+AESGCM:EECDH+ECDSA+SHA384:EECDH+ECDSA+SHA256:AES256:" \
                  "!DHE:!RSA:!AES128:!RC4:!DES:!3DES:!DSS:!SRP:!PSK:!EXP:!MD5:!LOW:!aNULL:!eNULL") != 1)
                  FC_THROW("Failed to set HTTPS cipher list");
            } catch (const fc::exception& e) {
               elog("https server initialization error: ${w}", ("w", e.to_detail_string()));
            } catch(std::exception& e) {
               elog("https server initialization error: ${w}", ("w", e.what()));
            }

            return ctx;
         }

         template<class T>
         static void handle_exception(typename websocketpp::server<detail::asio_with_stub_log<T>>::connection_ptr con) {
            string err = "Internal Service error, http: ";
            try {
               con->set_status( websocketpp::http::status_code::internal_server_error );
               try {
                  throw;
               } catch (const fc::exception& e) {
                  err += e.to_detail_string();
                  elog( "${e}", ("e", err));
                  error_results results{websocketpp::http::status_code::internal_server_error,
                                        "Internal Service Error", e};
                  con->set_body( fc::json::to_string( results ));
               } catch (const std::exception& e) {
                  err += e.what();
                  elog( "${e}", ("e", err));
                  error_results results{websocketpp::http::status_code::internal_server_error,
                                        "Internal Service Error", fc::exception( FC_LOG_MESSAGE( error, e.what()))};
                  con->set_body( fc::json::to_string( results ));
               } catch (...) {
                  err += "Unknown Exception";
                  error_results results{websocketpp::http::status_code::internal_server_error,
                                        "Internal Service Error",
                                        fc::exception( FC_LOG_MESSAGE( error, "Unknown Exception" ))};
                  con->set_body( fc::json::to_string( results ));
               }
            } catch (...) {
               con->set_body( R"xxx({"message": "Internal Server Error"})xxx" );
               std::cerr << "Exception attempting to handle exception: " << err << std::endl;
            }
         }

         template<class T>
         void handle_http_request(typename websocketpp::server<detail::asio_with_stub_log<T>>::connection_ptr con) {
            try {
               if( !access_control_allow_origin.empty()) {
                  con->append_header( "Access-Control-Allow-Origin", access_control_allow_origin );
               }
               if( !access_control_allow_headers.empty()) {
                  con->append_header( "Access-Control-Allow-Headers", access_control_allow_headers );
               }
               if( !access_control_max_age.empty()) {
                  con->append_header( "Access-Control-Max-Age", access_control_max_age );
               }
               if( access_control_allow_credentials ) {
                  con->append_header( "Access-Control-Allow-Credentials", "true" );
               }
               
               auto& req = con->get_request();
               if(req.get_method() == "OPTIONS") {
                  con->set_status(websocketpp::http::status_code::ok);
                  return;
               }

               con->append_header( "Content-type", "application/json" );
               auto body = con->get_request_body();
               auto resource = con->get_uri()->get_resource();
               auto handler_itr = url_handlers.find( resource );
               if( handler_itr != url_handlers.end()) {
                  con->defer_http_response();
                  handler_itr->second( resource, body, [con]( auto code, auto&& body ) {
                     con->set_body( std::move( body ));
                     con->set_status( websocketpp::http::status_code::value( code ));
                     con->send_http_response();
                  } );

               } else {
                  wlog( "404 - not found: ${ep}", ("ep", resource));
                  error_results results{websocketpp::http::status_code::not_found,
                                        "Not Found", fc::exception( FC_LOG_MESSAGE( error, "Unknown Endpoint" ))};
                  con->set_body( fc::json::to_string( results ));
                  con->set_status( websocketpp::http::status_code::not_found );
               }
            } catch( ... ) {
               handle_exception<T>( con );
            }
         }

         template<class T>
         void create_server_for_endpoint(const tcp::endpoint& ep, websocketpp::server<detail::asio_with_stub_log<T>>& ws) {
            try {
               ws.clear_access_channels(websocketpp::log::alevel::all);
               ws.init_asio(&app().get_io_service());
               ws.set_reuse_addr(true);
               //  1MB blocks plus some additional room for encryption if enabled plus message overhead,
               //  add an addition 512k to be conservative. Overrides the default 32MB.
               ws.set_max_http_body_size(1024*1024 + 512*1024);
               ws.set_max_message_size(1024*1024 + 512*1024);

               ws.set_http_handler([&](connection_hdl hdl) {
                  handle_http_request<T>(ws.get_con_from_hdl(hdl));
               });
            } catch ( const fc::exception& e ){
               elog( "http: ${e}", ("e",e.to_detail_string()));
            } catch ( const std::exception& e ){
               elog( "http: ${e}", ("e",e.what()));
            } catch (...) {
               elog("error thrown from http io service");
            }
         }
   };

   http_plugin::http_plugin():my(new http_plugin_impl()){}
   http_plugin::~http_plugin(){}

   void http_plugin::set_program_options(options_description&, options_description& cfg) {
      cfg.add_options()
            ("http-server-address", bpo::value<string>()->default_value("127.0.0.1:8888"),
             "The local IP and port to listen for incoming http connections; set blank to disable.")

            ("https-server-address", bpo::value<string>(),
             "The local IP and port to listen for incoming https connections; leave blank to disable.")

            ("https-certificate-chain-file", bpo::value<string>(),
             "Filename with the certificate chain to present on https connections. PEM format. Required for https.")

            ("https-private-key-file", bpo::value<string>(),
             "Filename with https private key in PEM format. Required for https")

            ("access-control-allow-origin", bpo::value<string>()->notifier([this](const string& v) {
                my->access_control_allow_origin = v;
                ilog("configured http with Access-Control-Allow-Origin: ${o}", ("o", my->access_control_allow_origin));
             }),
             "Specify the Access-Control-Allow-Origin to be returned on each request.")

            ("access-control-allow-headers", bpo::value<string>()->notifier([this](const string& v) {
                my->access_control_allow_headers = v;
                ilog("configured http with Access-Control-Allow-Headers : ${o}", ("o", my->access_control_allow_headers));
             }),
             "Specify the Access-Control-Allow-Headers to be returned on each request.")

            ("access-control-max-age", bpo::value<string>()->notifier([this](const string& v) {
                my->access_control_max_age = v;
                ilog("configured http with Access-Control-Max-Age : ${o}", ("o", my->access_control_max_age));
             }),
             "Specify the Access-Control-Max-Age to be returned on each request.")

            ("access-control-allow-credentials",
             bpo::bool_switch()->notifier([this](bool v) {
                my->access_control_allow_credentials = v;
                if (v) ilog("configured http with Access-Control-Allow-Credentials: true");
             })->default_value(false),
             "Specify if Access-Control-Allow-Credentials: true should be returned on each request.")
            ;
   }

   void http_plugin::plugin_initialize(const variables_map& options) {
      tcp::resolver resolver(app().get_io_service());
      if(options.count("http-server-address") && options.at("http-server-address").as<string>().length()) {
         string lipstr =  options.at("http-server-address").as<string>();
         string host = lipstr.substr(0, lipstr.find(':'));
         string port = lipstr.substr(host.size()+1, lipstr.size());
         tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str() );
         try {
            my->listen_endpoint = *resolver.resolve(query);
            ilog("configured http to listen on ${h}:${p}", ("h",host)("p",port));
         } catch(const boost::system::system_error& ec) {
            elog("failed to configure http to listen on ${h}:${p} (${m})", ("h",host)("p",port)("m", ec.what()));
         }
      }

      if(options.count("https-server-address") && options.at("https-server-address").as<string>().length()) {
         if(!options.count("https-certificate-chain-file") || options.at("https-certificate-chain-file").as<string>().empty()) {
            elog("https-certificate-chain-file is required for HTTPS");
            return;
         }
         if(!options.count("https-private-key-file") || options.at("https-private-key-file").as<string>().empty()) {
            elog("https-private-key-file is required for HTTPS");
            return;
         }

         string lipstr =  options.at("https-server-address").as<string>();
         string host = lipstr.substr(0, lipstr.find(':'));
         string port = lipstr.substr(host.size()+1, lipstr.size());
         tcp::resolver::query query(tcp::v4(), host.c_str(), port.c_str());
         try {
            my->https_listen_endpoint = *resolver.resolve(query);
            ilog("configured https to listen on ${h}:${p} (TLS configuration will be validated momentarily)", ("h",host)("p",port));
            my->https_cert_chain = options.at("https-certificate-chain-file").as<string>();
            my->https_key = options.at("https-private-key-file").as<string>();
         } catch(const boost::system::system_error& ec) {
            elog("failed to configure https to listen on ${h}:${p} (${m})", ("h",host)("p",port)("m", ec.what()));
         }
      }

      //watch out for the returns above when adding new code here
   }

   void http_plugin::plugin_startup() {
      if(my->listen_endpoint) {
         try {
            my->create_server_for_endpoint(*my->listen_endpoint, my->server);

            ilog("start listening for http requests");
            my->server.listen(*my->listen_endpoint);
            my->server.start_accept();
         } catch ( const fc::exception& e ){
            elog( "http service failed to start: ${e}", ("e",e.to_detail_string()));
            throw;
         } catch ( const std::exception& e ){
            elog( "http service failed to start: ${e}", ("e",e.what()));
            throw;
         } catch (...) {
            elog("error thrown from http io service");
            throw;
         }
      }

      if(my->https_listen_endpoint) {
         try {
            my->create_server_for_endpoint(*my->https_listen_endpoint, my->https_server);
            my->https_server.set_tls_init_handler([this](websocketpp::connection_hdl hdl) -> ssl_context_ptr{
               return my->on_tls_init(hdl);
            });

            ilog("start listening for https requests");
            my->https_server.listen(*my->https_listen_endpoint);
            my->https_server.start_accept();
         } catch ( const fc::exception& e ){
            elog( "https service failed to start: ${e}", ("e",e.to_detail_string()));
            throw;
         } catch ( const std::exception& e ){
            elog( "https service failed to start: ${e}", ("e",e.what()));
            throw;
         } catch (...) {
            elog("error thrown from https io service");
            throw;
         }
      }
   }

   void http_plugin::plugin_shutdown() {
      if(my->server.is_listening())
         my->server.stop_listening();
      if(my->https_server.is_listening())
         my->https_server.stop_listening();
   }

   void http_plugin::add_handler(const string& url, const url_handler& handler) {
      ilog( "add api url: ${c}", ("c",url) );
      app().get_io_service().post([=](){
        my->url_handlers.insert(std::make_pair(url,handler));
      });
   }

   void http_plugin::handle_exception( const char *api_name, const char *call_name, const string& body, url_response_callback cb ) {
      try {
         try {
            throw;
         } catch (chain::unsatisfied_authorization& e) {
            error_results results{401, "UnAuthorized", e};
            cb( 401, fc::json::to_string( results ));
         } catch (chain::tx_duplicate& e) {
            error_results results{409, "Conflict", e};
            cb( 409, fc::json::to_string( results ));
         } catch (chain::transaction_exception& e) {
            error_results results{400, "Bad Request", e};
            cb( 400, fc::json::to_string( results ));
         } catch (fc::eof_exception& e) {
            error_results results{400, "Bad Request", e};
            cb( 400, fc::json::to_string( results ));
            elog( "Unable to parse arguments: ${args}", ("args", body));
         } catch (fc::exception& e) {
            error_results results{500, "Internal Service Error", e};
            cb( 500, fc::json::to_string( results ));
            elog( "FC Exception encountered while processing ${api}.${call}: ${e}",
                  ("api", api_name)( "call", call_name )( "e", e.to_detail_string()));
         } catch (std::exception& e) {
            error_results results{500, "Internal Service Error", fc::exception( FC_LOG_MESSAGE( error, e.what()))};
            cb( 500, fc::json::to_string( results ));
            elog( "STD Exception encountered while processing ${api}.${call}: ${e}",
                  ("api", api_name)( "call", call_name )( "e", e.what()));
         } catch (...) {
            error_results results{500, "Internal Service Error",
                                  fc::exception( FC_LOG_MESSAGE( error, "Unknown Exception" ))};
            cb( 500, fc::json::to_string( results ));
            elog( "Unknown Exception encountered while processing ${api}.${call}",
                  ("api", api_name)( "call", call_name ));
         }
      } catch (...) {
         std::cerr << "Exception attempting to handle exception for " << api_name << "." << call_name << std::endl;
      }
   }

}
