/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/http_plugin/local_endpoint.hpp>
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
#include <regex>

namespace eosio {

   static appbase::abstract_plugin& _http_plugin = app().register_plugin<http_plugin>();

   namespace asio = boost::asio;

   using std::map;
   using std::vector;
   using std::set;
   using std::string;
   using std::regex;
   using boost::optional;
   using boost::asio::ip::tcp;
   using boost::asio::ip::address_v4;
   using boost::asio::ip::address_v6;
   using std::shared_ptr;
   using websocketpp::connection_hdl;

   static http_plugin_defaults current_http_plugin_defaults;

   void http_plugin::set_defaults(const http_plugin_defaults config) {
      current_http_plugin_defaults = config;
   }

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

      struct asio_local_with_stub_log : public websocketpp::config::asio {
          typedef asio_local_with_stub_log type;
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
              typedef websocketpp::transport::asio::basic_socket::local_endpoint socket_type;
          };

          typedef websocketpp::transport::asio::local_endpoint<transport_config> transport_type;

          static const long timeout_open_handshake = 0;
      };
   }

   using websocket_server_type = websocketpp::server<detail::asio_with_stub_log<websocketpp::transport::asio::basic_socket::endpoint>>;
   using websocket_local_server_type = websocketpp::server<detail::asio_local_with_stub_log>;
   using websocket_server_tls_type =  websocketpp::server<detail::asio_with_stub_log<websocketpp::transport::asio::tls_socket::endpoint>>;
   using ssl_context_ptr =  websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;
   using io_work_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;

   static bool verbose_http_errors = false;

   class http_plugin_impl {
      public:
         map<string,url_handler>  url_handlers;
         optional<tcp::endpoint>  listen_endpoint;
         string                   access_control_allow_origin;
         string                   access_control_allow_headers;
         string                   access_control_max_age;
         bool                     access_control_allow_credentials = false;
         size_t                   max_body_size;

         websocket_server_type    server;

         uint16_t                                    thread_pool_size = 2;
         optional<boost::asio::thread_pool>          thread_pool;
         std::shared_ptr<boost::asio::io_context>    server_ioc;
         optional<io_work_t>                         server_ioc_work;
         std::atomic<int64_t>                        bytes_in_flight{0};
         size_t                                      max_bytes_in_flight = 0;

         optional<tcp::endpoint>  https_listen_endpoint;
         string                   https_cert_chain;
         string                   https_key;

         websocket_server_tls_type https_server;

         optional<asio::local::stream_protocol::endpoint> unix_endpoint;
         websocket_local_server_type unix_server;

         bool                     validate_host;
         set<string>              valid_hosts;

         bool host_port_is_valid( const std::string& header_host_port, const string& endpoint_local_host_port ) {
            return !validate_host || header_host_port == endpoint_local_host_port || valid_hosts.find(header_host_port) != valid_hosts.end();
         }

         bool host_is_valid( const std::string& host, const string& endpoint_local_host_port, bool secure) {
            if (!validate_host) {
               return true;
            }

            // normalise the incoming host so that it always has the explicit port
            static auto has_port_expr = regex("[^:]:[0-9]+$"); /// ends in :<number> without a preceeding colon which implies ipv6
            if (std::regex_search(host, has_port_expr)) {
               return host_port_is_valid( host, endpoint_local_host_port );
            } else {
               // according to RFC 2732 ipv6 addresses should always be enclosed with brackets so we shouldn't need to special case here
               return host_port_is_valid( host + ":" + std::to_string(secure ? websocketpp::uri_default_secure_port : websocketpp::uri_default_port ), endpoint_local_host_port);
            }
         }

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
                  EOS_THROW(chain::http_exception, "Failed to set NID_secp384r1");
               if(SSL_CTX_set_tmp_ecdh(ctx->native_handle(), (EC_KEY*)ecdh) != 1)
                  EOS_THROW(chain::http_exception, "Failed to set ECDH PFS");

               if(SSL_CTX_set_cipher_list(ctx->native_handle(), \
                  "EECDH+ECDSA+AESGCM:EECDH+aRSA+AESGCM:EECDH+ECDSA+SHA384:EECDH+ECDSA+SHA256:AES256:" \
                  "!DHE:!RSA:!AES128:!RC4:!DES:!3DES:!DSS:!SRP:!PSK:!EXP:!MD5:!LOW:!aNULL:!eNULL") != 1)
                  EOS_THROW(chain::http_exception, "Failed to set HTTPS cipher list");
            } catch (const fc::exception& e) {
               elog("https server initialization error: ${w}", ("w", e.to_detail_string()));
            } catch(std::exception& e) {
               elog("https server initialization error: ${w}", ("w", e.what()));
            }

            return ctx;
         }

         template<class T>
         static void handle_exception(typename websocketpp::server<T>::connection_ptr con) {
            string err = "Internal Service error, http: ";
            try {
               con->set_status( websocketpp::http::status_code::internal_server_error );
               try {
                  throw;
               } catch (const fc::exception& e) {
                  err += e.to_detail_string();
                  elog( "${e}", ("e", err));
                  error_results results{websocketpp::http::status_code::internal_server_error,
                                        "Internal Service Error", error_results::error_info(e, verbose_http_errors )};
                  con->set_body( fc::json::to_string( results ));
               } catch (const std::exception& e) {
                  err += e.what();
                  elog( "${e}", ("e", err));
                  error_results results{websocketpp::http::status_code::internal_server_error,
                                        "Internal Service Error", error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, e.what())), verbose_http_errors )};
                  con->set_body( fc::json::to_string( results ));
               } catch (...) {
                  err += "Unknown Exception";
                  error_results results{websocketpp::http::status_code::internal_server_error,
                                        "Internal Service Error",
                                        error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, "Unknown Exception" )), verbose_http_errors )};
                  con->set_body( fc::json::to_string( results ));
               }
            } catch (...) {
               con->set_body( R"xxx({"message": "Internal Server Error"})xxx" );
               std::cerr << "Exception attempting to handle exception: " << err << std::endl;
            }
         }

         template<class T>
         bool allow_host(const typename T::request_type& req, typename websocketpp::server<T>::connection_ptr con) {
            bool is_secure = con->get_uri()->get_secure();
            const auto& local_endpoint = con->get_socket().lowest_layer().local_endpoint();
            auto local_socket_host_port = local_endpoint.address().to_string() + ":" + std::to_string(local_endpoint.port());

            const auto& host_str = req.get_header("Host");
            if (host_str.empty() || !host_is_valid(host_str, local_socket_host_port, is_secure)) {
               con->set_status(websocketpp::http::status_code::bad_request);
               return false;
            }
            return true;
         }

         template<class T>
         void handle_http_request(typename websocketpp::server<T>::connection_ptr con) {
            try {
               auto& req = con->get_request();

               if(!allow_host<T>(req, con))
                  return;

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

               if(req.get_method() == "OPTIONS") {
                  con->set_status(websocketpp::http::status_code::ok);
                  return;
               }

               con->append_header( "Content-type", "application/json" );

               if( bytes_in_flight > max_bytes_in_flight ) {
                  dlog( "503 - too many bytes in flight: ${bytes}", ("bytes", bytes_in_flight.load()) );
                  error_results results{websocketpp::http::status_code::too_many_requests, "Busy", error_results::error_info()};
                  con->set_body( fc::json::to_string( results ));
                  con->set_status( websocketpp::http::status_code::too_many_requests );
                  return;
               }

               std::string body = con->get_request_body();
               std::string resource = con->get_uri()->get_resource();
               auto handler_itr = url_handlers.find( resource );
               if( handler_itr != url_handlers.end()) {
                  con->defer_http_response();
                  bytes_in_flight += body.size();
                  app().post( appbase::priority::low,
                              [ioc = this->server_ioc, &bytes_in_flight = this->bytes_in_flight, handler_itr,
                               resource{std::move( resource )}, body{std::move( body )}, con]() {
                     try {
                        bytes_in_flight -= body.size();
                        handler_itr->second( resource, body,
                              [ioc{std::move(ioc)}, &bytes_in_flight, con]( int code, std::string response_body ) {
                           bytes_in_flight += response_body.size();
                           boost::asio::post( *ioc, [ioc, response_body{std::move( response_body )}, &bytes_in_flight, con, code]() {
                              size_t body_size = response_body.size();
                              con->set_body( std::move( response_body ) );
                              con->set_status( websocketpp::http::status_code::value( code ) );
                              con->send_http_response();
                              bytes_in_flight -= body_size;
                           } );
                        });
                     } catch( ... ) {
                        handle_exception<T>( con );
                        con->send_http_response();
                     }
                  } );

               } else {
                  dlog( "404 - not found: ${ep}", ("ep", resource));
                  error_results results{websocketpp::http::status_code::not_found,
                                        "Not Found", error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, "Unknown Endpoint" )), verbose_http_errors )};
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
               ws.init_asio(&(*server_ioc));
               ws.set_reuse_addr(true);
               ws.set_max_http_body_size(max_body_size);
               // capture server_ioc shared_ptr in http handler to keep it alive while in use
               ws.set_http_handler([&, ioc = this->server_ioc](connection_hdl hdl) {
                  handle_http_request<detail::asio_with_stub_log<T>>(ws.get_con_from_hdl(hdl));
               });
            } catch ( const fc::exception& e ){
               elog( "http: ${e}", ("e",e.to_detail_string()));
            } catch ( const std::exception& e ){
               elog( "http: ${e}", ("e",e.what()));
            } catch (...) {
               elog("error thrown from http io service");
            }
         }

         void add_aliases_for_endpoint( const tcp::endpoint& ep, string host, string port ) {
            auto resolved_port_str = std::to_string(ep.port());
            valid_hosts.emplace(host + ":" + port);
            valid_hosts.emplace(host + ":" + resolved_port_str);
         }
   };

   template<>
   bool http_plugin_impl::allow_host<detail::asio_local_with_stub_log>(const detail::asio_local_with_stub_log::request_type& req, websocketpp::server<detail::asio_local_with_stub_log>::connection_ptr con) {
      return true;
   }

   http_plugin::http_plugin():my(new http_plugin_impl()){}
   http_plugin::~http_plugin(){}

   void http_plugin::set_program_options(options_description&, options_description& cfg) {
      if(current_http_plugin_defaults.default_unix_socket_path.length())
         cfg.add_options()
            ("unix-socket-path", bpo::value<string>()->default_value(current_http_plugin_defaults.default_unix_socket_path),
             "The filename (relative to data-dir) to create a unix socket for HTTP RPC; set blank to disable.");

      if(current_http_plugin_defaults.default_http_port)
         cfg.add_options()
            ("http-server-address", bpo::value<string>()->default_value("127.0.0.1:" + std::to_string(current_http_plugin_defaults.default_http_port)),
             "The local IP and port to listen for incoming http connections; set blank to disable.");
      else
         cfg.add_options()
            ("http-server-address", bpo::value<string>(),
             "The local IP and port to listen for incoming http connections; leave blank to disable.");

      cfg.add_options()
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
            ("max-body-size", bpo::value<uint32_t>()->default_value(1024*1024),
             "The maximum body size in bytes allowed for incoming RPC requests")
            ("http-max-bytes-in-flight-mb", bpo::value<uint32_t>()->default_value(500),
             "Maximum size in megabytes http_plugin should use for processing http requests. 503 error response when exceeded." )
            ("verbose-http-errors", bpo::bool_switch()->default_value(false),
             "Append the error log to HTTP responses")
            ("http-validate-host", boost::program_options::value<bool>()->default_value(true),
             "If set to false, then any incoming \"Host\" header is considered valid")
            ("http-alias", bpo::value<std::vector<string>>()->composing(),
             "Additionaly acceptable values for the \"Host\" header of incoming HTTP requests, can be specified multiple times.  Includes http/s_server_address by default.")
            ("http-threads", bpo::value<uint16_t>()->default_value( my->thread_pool_size ),
             "Number of worker threads in http thread pool")
            ;
   }

   void http_plugin::plugin_initialize(const variables_map& options) {
      try {
         my->validate_host = options.at("http-validate-host").as<bool>();
         if( options.count( "http-alias" )) {
            const auto& aliases = options["http-alias"].as<vector<string>>();
            my->valid_hosts.insert(aliases.begin(), aliases.end());
         }

         tcp::resolver resolver( app().get_io_service());
         if( options.count( "http-server-address" ) && options.at( "http-server-address" ).as<string>().length()) {
            string lipstr = options.at( "http-server-address" ).as<string>();
            string host = lipstr.substr( 0, lipstr.find( ':' ));
            string port = lipstr.substr( host.size() + 1, lipstr.size());
            tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str());
            try {
               my->listen_endpoint = *resolver.resolve( query );
               ilog( "configured http to listen on ${h}:${p}", ("h", host)( "p", port ));
            } catch ( const boost::system::system_error& ec ) {
               elog( "failed to configure http to listen on ${h}:${p} (${m})",
                     ("h", host)( "p", port )( "m", ec.what()));
            }

            // add in resolved hosts and ports as well
            if (my->listen_endpoint) {
               my->add_aliases_for_endpoint(*my->listen_endpoint, host, port);
            }
         }

         if( options.count( "unix-socket-path" ) && !options.at( "unix-socket-path" ).as<string>().empty()) {
            boost::filesystem::path sock_path = options.at("unix-socket-path").as<string>();
            if (sock_path.is_relative())
               sock_path = app().data_dir() / sock_path;
            my->unix_endpoint = asio::local::stream_protocol::endpoint(sock_path.string());
         }

         if( options.count( "https-server-address" ) && options.at( "https-server-address" ).as<string>().length()) {
            if( !options.count( "https-certificate-chain-file" ) ||
                options.at( "https-certificate-chain-file" ).as<string>().empty()) {
               elog( "https-certificate-chain-file is required for HTTPS" );
               return;
            }
            if( !options.count( "https-private-key-file" ) ||
                options.at( "https-private-key-file" ).as<string>().empty()) {
               elog( "https-private-key-file is required for HTTPS" );
               return;
            }

            string lipstr = options.at( "https-server-address" ).as<string>();
            string host = lipstr.substr( 0, lipstr.find( ':' ));
            string port = lipstr.substr( host.size() + 1, lipstr.size());
            tcp::resolver::query query( tcp::v4(), host.c_str(), port.c_str());
            try {
               my->https_listen_endpoint = *resolver.resolve( query );
               ilog( "configured https to listen on ${h}:${p} (TLS configuration will be validated momentarily)",
                     ("h", host)( "p", port ));
               my->https_cert_chain = options.at( "https-certificate-chain-file" ).as<string>();
               my->https_key = options.at( "https-private-key-file" ).as<string>();
            } catch ( const boost::system::system_error& ec ) {
               elog( "failed to configure https to listen on ${h}:${p} (${m})",
                     ("h", host)( "p", port )( "m", ec.what()));
            }

            // add in resolved hosts and ports as well
            if (my->https_listen_endpoint) {
               my->add_aliases_for_endpoint(*my->https_listen_endpoint, host, port);
            }
         }

         my->max_body_size = options.at( "max-body-size" ).as<uint32_t>();
         verbose_http_errors = options.at( "verbose-http-errors" ).as<bool>();

         my->thread_pool_size = options.at( "http-threads" ).as<uint16_t>();
         EOS_ASSERT( my->thread_pool_size > 0, chain::plugin_config_exception,
                     "http-threads ${num} must be greater than 0", ("num", my->thread_pool_size));

         my->max_bytes_in_flight = options.at( "http-max-bytes-in-flight-mb" ).as<uint32_t>() * 1024 * 1024;

         //watch out for the returns above when adding new code here
      } FC_LOG_AND_RETHROW()
   }

   void http_plugin::plugin_startup() {

      my->thread_pool.emplace( my->thread_pool_size );
      my->server_ioc = std::make_shared<boost::asio::io_context>();
      my->server_ioc_work.emplace( boost::asio::make_work_guard(*my->server_ioc) );
      for( uint16_t i = 0; i < my->thread_pool_size; ++i ) {
         boost::asio::post( *my->thread_pool, [ioc = my->server_ioc]() { ioc->run(); } );
      }

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

      if(my->unix_endpoint) {
         try {
            my->unix_server.clear_access_channels(websocketpp::log::alevel::all);
            my->unix_server.init_asio(&(*my->server_ioc));
            my->unix_server.set_max_http_body_size(my->max_body_size);
            my->unix_server.listen(*my->unix_endpoint);
            my->unix_server.set_http_handler([&, ioc = my->server_ioc](connection_hdl hdl) {
               my->handle_http_request<detail::asio_local_with_stub_log>( my->unix_server.get_con_from_hdl(hdl));
            });
            my->unix_server.start_accept();
         } catch ( const fc::exception& e ){
            elog( "unix socket service failed to start: ${e}", ("e",e.to_detail_string()));
            throw;
         } catch ( const std::exception& e ){
            elog( "unix socket service failed to start: ${e}", ("e",e.what()));
            throw;
         } catch (...) {
            elog("error thrown from unix socket io service");
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

      add_api({{
         std::string("/v1/node/get_supported_apis"),
         [&](string, string body, url_response_callback cb) mutable {
            try {
               if (body.empty()) body = "{}";
               auto result = (*this).get_supported_apis();
               cb(200, fc::json::to_string(result));
            } catch (...) {
               handle_exception("node", "get_supported_apis", body, cb);
            }
         }
      }});
   }

   void http_plugin::plugin_shutdown() {
      if(my->server.is_listening())
         my->server.stop_listening();
      if(my->https_server.is_listening())
         my->https_server.stop_listening();
      if(my->unix_server.is_listening())
         my->unix_server.stop_listening();

      if( my->server_ioc_work )
         my->server_ioc_work->reset();
      if( my->server_ioc )
         my->server_ioc->stop();
      if( my->thread_pool ) {
         my->thread_pool->join();
         my->thread_pool->stop();
      }
   }

   void http_plugin::add_handler(const string& url, const url_handler& handler) {
      ilog( "add api url: ${c}", ("c",url) );
      my->url_handlers.insert(std::make_pair(url,handler));
   }

   void http_plugin::handle_exception( const char *api_name, const char *call_name, const string& body, url_response_callback cb ) {
      try {
         try {
            throw;
         } catch (chain::unknown_block_exception& e) {
            error_results results{400, "Unknown Block", error_results::error_info(e, verbose_http_errors)};
            cb( 400, fc::json::to_string( results ));
         } catch (chain::unsatisfied_authorization& e) {
            error_results results{401, "UnAuthorized", error_results::error_info(e, verbose_http_errors)};
            cb( 401, fc::json::to_string( results ));
         } catch (chain::tx_duplicate& e) {
            error_results results{409, "Conflict", error_results::error_info(e, verbose_http_errors)};
            cb( 409, fc::json::to_string( results ));
         } catch (fc::eof_exception& e) {
            error_results results{422, "Unprocessable Entity", error_results::error_info(e, verbose_http_errors)};
            cb( 422, fc::json::to_string( results ));
            elog( "Unable to parse arguments to ${api}.${call}", ("api", api_name)( "call", call_name ));
            dlog("Bad arguments: ${args}", ("args", body));
         } catch (fc::exception& e) {
            error_results results{500, "Internal Service Error", error_results::error_info(e, verbose_http_errors)};
            cb( 500, fc::json::to_string( results ));
            if (e.code() != chain::greylist_net_usage_exceeded::code_value && e.code() != chain::greylist_cpu_usage_exceeded::code_value) {
               elog( "FC Exception encountered while processing ${api}.${call}",
                     ("api", api_name)( "call", call_name ));
               dlog( "Exception Details: ${e}", ("e", e.to_detail_string()));
            }
         } catch (std::exception& e) {
            error_results results{500, "Internal Service Error", error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, e.what())), verbose_http_errors)};
            cb( 500, fc::json::to_string( results ));
            elog( "STD Exception encountered while processing ${api}.${call}",
                  ("api", api_name)( "call", call_name ));
            dlog( "Exception Details: ${e}", ("e", e.what()));
         } catch (...) {
            error_results results{500, "Internal Service Error",
               error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, "Unknown Exception" )), verbose_http_errors)};
            cb( 500, fc::json::to_string( results ));
            elog( "Unknown Exception encountered while processing ${api}.${call}",
                  ("api", api_name)( "call", call_name ));
         }
      } catch (...) {
         std::cerr << "Exception attempting to handle exception for " << api_name << "." << call_name << std::endl;
      }
   }

   bool http_plugin::is_on_loopback() const {
      return (!my->listen_endpoint || my->listen_endpoint->address().is_loopback()) && (!my->https_listen_endpoint || my->https_listen_endpoint->address().is_loopback());
   }

   bool http_plugin::is_secure() const {
      return (!my->listen_endpoint || my->listen_endpoint->address().is_loopback());
   }

   bool http_plugin::verbose_errors()const {
      return verbose_http_errors;
   }

   http_plugin::get_supported_apis_result http_plugin::get_supported_apis()const {
      get_supported_apis_result result;

      for (const auto& handler : my->url_handlers) {
         if (handler.first != "/v1/node/get_supported_apis")
            result.apis.emplace_back(handler.first);
      }

      return result;
   }
}
