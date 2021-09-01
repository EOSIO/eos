#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/network/ip.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/io/raw.hpp>
#include <fc/crypto/openssl.hpp>

#include <boost/asio.hpp>
#include <boost/optional.hpp>

#include <thread>
#include <memory>
#include <regex>

#include "common.hpp"
#include "beast_http_listener.hpp"

const fc::string logger_name("http_plugin");
fc::logger logger;

namespace eosio {

   static appbase::abstract_plugin& _http_plugin = app().register_plugin<http_plugin>();

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
   
   enum https_ecdh_curve_t {
      SECP384R1,
      PRIME256V1
   };

   static http_plugin_defaults current_http_plugin_defaults;
   static bool verbose_http_errors;

   void http_plugin::set_defaults(const http_plugin_defaults& config) {
      current_http_plugin_defaults = config;
   }

   using http_plugin_impl_ptr = std::shared_ptr<class http_plugin_impl>;

class http_plugin_impl : public std::enable_shared_from_this<http_plugin_impl> {
      public:
         http_plugin_impl() = default;

         http_plugin_impl(const http_plugin_impl&) = delete;
         http_plugin_impl(http_plugin_impl&&) = delete;

         http_plugin_impl& operator=(const http_plugin_impl&) = delete;
         http_plugin_impl& operator=(http_plugin_impl&&) = delete;
         
         std::optional<tcp::endpoint>  listen_endpoint;
         
         uint16_t                                       thread_pool_size = 2;
         std::shared_ptr<eosio::chain::named_thread_pool> thread_pool;
         
         std::optional<tcp::endpoint>  https_listen_endpoint;
         string                        https_cert_chain;
         string                        https_key;
         https_ecdh_curve_t            https_ecdh_curve = SECP384R1;

         std::optional<asio::local::stream_protocol::endpoint> unix_endpoint;

         shared_ptr<beast_http_listener<plain_session, tcp, tcp_socket_t > >  beast_server;
         shared_ptr<beast_http_listener<ssl_session, tcp, tcp_socket_t > >  beast_https_server;
         shared_ptr<beast_http_listener<unix_socket_session, stream_protocol, stream_protocol::socket > > beast_unix_server;

         shared_ptr<http_plugin_state> plugin_state = std::make_shared<http_plugin_state>();

         /**
          * Make an internal_url_handler that will run the url_handler on the app() thread and then
          * return to the http thread pool for response processing
          *
          * @pre b.size() has been added to bytes_in_flight by caller
          * @param priority - priority to post to the app thread at
          * @param next - the next handler for responses
          * @param my - the http_plugin_impl
          * @return the constructed internal_url_handler
          */
         static detail::internal_url_handler make_app_thread_url_handler( int priority, url_handler next, http_plugin_impl_ptr my ) {
            auto next_ptr = std::make_shared<url_handler>(std::move(next));
            return [my=std::move(my), priority, next_ptr=std::move(next_ptr)]
                       ( detail::abstract_conn_ptr conn, string r, string b, url_response_callback then ) {
               auto tracked_b = make_in_flight<string>(std::move(b), *(my->plugin_state));
               if (!conn->verify_max_bytes_in_flight()) {
                  return;
               }

               url_response_callback wrapped_then = [tracked_b, then=std::move(then)](int code, std::optional<fc::variant> resp) {
                  then(code, std::move(resp));
               };

               // post to the app thread taking shared ownership of next (via std::shared_ptr),
               // sole ownership of the tracked body and the passed in parameters
               app().post( priority, [next_ptr, conn=std::move(conn), r=std::move(r), tracked_b, wrapped_then=std::move(wrapped_then)]() mutable {
                  try {
                     // call the `next` url_handler and wrap the response handler
                     (*next_ptr)( std::move( r ), std::move(tracked_b->obj()), std::move(wrapped_then)) ;
                  } catch( ... ) {
                     conn->handle_exception();
                  }
               } );
            };
         }

         /**
          * Make an internal_url_handler that will run the url_handler directly
          *
          * @pre b.size() has been added to bytes_in_flight by caller
          * @param next - the next handler for responses
          * @return the constructed internal_url_handler
          */
         static detail::internal_url_handler make_http_thread_url_handler(url_handler next) {
            return [next=std::move(next)]( const detail::abstract_conn_ptr& conn, string r, string b, url_response_callback then ) {
               try {
                  next(std::move(r), std::move(b), std::move(then));
               } catch( ... ) {
                  conn->handle_exception();
               }
             };
         }

         void add_aliases_for_endpoint( const tcp::endpoint& ep, host_port_t host_port) {
            auto alias = std::make_pair(ep.address().to_string(), ep.port());
            ilog( "adding host:'${h}' port:${p} alias: ${ah} port: ${ap}",
                   ("h", host_port.first)("p", host_port.second)("ah", alias.first)("ap", alias.second));
            
            plugin_state->valid_hosts.emplace(host_port);
            plugin_state->valid_hosts.emplace(alias);
            try {
               auto addr_ipv6 = ep.address().to_v6().to_string(); 
               plugin_state->valid_hosts.emplace(std::make_pair(addr_ipv6, ep.port()));
               ilog("adding host:'${h}' port:${p} alias: ${ah} port: ${ap}",
                  ("h", host_port.first)("p", host_port.second)("ah", addr_ipv6)("ap", ep.port()));
            } catch (  const std::exception& e ) {
               // do nithing if conversion fails
            }
         }

         void create_beast_server(bool useSSL, bool isUnix=false) {
            // beast_ssl_ctx = {ssl::context::tlsv12}
            auto ctx = std::make_shared<ssl::context>(ssl::context::tlsv12);
            
            if(useSSL) {
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

                  fc::ec_key ecdh = EC_KEY_new_by_curve_name(https_ecdh_curve == SECP384R1 ? NID_secp384r1 : NID_X9_62_prime256v1);
                  if (!ecdh)
                     EOS_THROW(chain::http_exception, "Failed to set NID_secp384r1");
                  if(SSL_CTX_set_tmp_ecdh(ctx->native_handle(), (EC_KEY*)ecdh) != 1)
                     EOS_THROW(chain::http_exception, "Failed to set ECDH PFS");

                  if(SSL_CTX_set_cipher_list(ctx->native_handle(), \
                     "EECDH+ECDSA+AESGCM:EECDH+aRSA+AESGCM:EECDH+ECDSA+SHA384:EECDH+ECDSA+SHA256:AES256:" \
                     "!DHE:!RSA:!AES128:!RC4:!DES:!3DES:!DSS:!SRP:!PSK:!EXP:!MD5:!LOW:!aNULL:!eNULL") != 1)
                     EOS_THROW(chain::http_exception, "Failed to set HTTPS cipher list");
               } catch (const fc::exception& e) {
                  fc_elog( logger, "https server initialization error: ${w}", ("w", e.to_detail_string()) );
               } catch(std::exception& e) {
                  fc_elog( logger, "https server initialization error: ${w}", ("w", e.what()) );
               }

               beast_https_server = std::make_shared<beast_http_listener<ssl_session, tcp, tcp_socket_t> >(thread_pool, ctx, plugin_state);
               fc_ilog( logger, "created beast HTTPS listener");
            }
            else {
               if(isUnix) {
                  beast_unix_server = std::make_shared<beast_http_listener<unix_socket_session, stream_protocol, stream_protocol::socket> >(thread_pool, ctx, plugin_state);
                  fc_ilog( logger, "created beast UNIX socket listener");
               }
               else {
                  beast_server = std::make_shared<beast_http_listener<plain_session, tcp, tcp_socket_t> >(thread_pool, ctx, plugin_state);
                  fc_ilog( logger, "created beast HTTP listener");
               }
            }
         }
   };

   http_plugin::http_plugin():my(new http_plugin_impl()){
      app().register_config_type<https_ecdh_curve_t>();
   }
   http_plugin::~http_plugin() = default;

   void http_plugin::set_program_options(options_description&, options_description& cfg) {
      if(current_http_plugin_defaults.default_unix_socket_path.length())
         cfg.add_options()
            ("unix-socket-path", bpo::value<string>()->default_value(current_http_plugin_defaults.default_unix_socket_path),
             "The filename (relative to data-dir) to create a unix socket for HTTP RPC; set blank to disable.");
      else
         cfg.add_options()
            ("unix-socket-path", bpo::value<string>(),
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

            ("https-ecdh-curve", bpo::value<https_ecdh_curve_t>()->notifier([this](https_ecdh_curve_t c) {
               my->https_ecdh_curve = c;
            })->default_value(SECP384R1),
            "Configure https ECDH curve to use: secp384r1 or prime256v1")

            ("access-control-allow-origin", bpo::value<string>()->notifier([this](const string& v) {
                my->plugin_state->access_control_allow_origin = v;
                fc_ilog( logger, "configured http with Access-Control-Allow-Origin: ${o}",
                         ("o", my->plugin_state->access_control_allow_origin) );
             }),
             "Specify the Access-Control-Allow-Origin to be returned on each requeplugin_state->")

            ("access-control-allow-headers", bpo::value<string>()->notifier([this](const string& v) {
                my->plugin_state->access_control_allow_headers = v;
                fc_ilog( logger, "configured http with Access-Control-Allow-Headers : ${o}",
                         ("o", my->plugin_state->access_control_allow_headers) );
             }),
             "Specify the Access-Control-Allow-Headers to be returned on each requeplugin_state->")

            ("access-control-max-age", bpo::value<string>()->notifier([this](const string& v) {
                my->plugin_state->access_control_max_age = v;
                fc_ilog( logger, "configured http with Access-Control-Max-Age : ${o}",
                         ("o", my->plugin_state->access_control_max_age) );
             }),
             "Specify the Access-Control-Max-Age to be returned on each request.")

            ("access-control-allow-credentials",
             bpo::bool_switch()->notifier([this](bool v) {
                my->plugin_state->access_control_allow_credentials = v;
                if( v ) fc_ilog( logger, "configured http with Access-Control-Allow-Credentials: true" );
             })->default_value(false),
             "Specify if Access-Control-Allow-Credentials: true should be returned on each request.")
            ("max-body-size", bpo::value<uint32_t>()->default_value(1024*1024),
             "The maximum body size in bytes allowed for incoming RPC requests")
            ("http-max-bytes-in-flight-mb", bpo::value<uint32_t>()->default_value(500),
             "Maximum size in megabytes http_plugin should use for processing http requests. 429 error response when exceeded." )
            ("http-max-in-flight-requests", bpo::value<int32_t>()->default_value(-1),
             "Maximum number of requests http_plugin should use for processing http requests. 429 error response when exceeded." )
            ("http-max-response-time-ms", bpo::value<uint32_t>()->default_value(30),
             "Maximum time for processing a request.")
            ("verbose-http-errors", bpo::bool_switch()->default_value(false),
             "Append the error log to HTTP responses")
            ("http-validate-host", boost::program_options::value<bool>()->default_value(true),
             "If set to false, then any incoming \"Host\" header is considered valid")
            ("http-alias", bpo::value<std::vector<string>>()->composing(),
             "Additionaly acceptable values for the \"Host\" header of incoming HTTP requests, can be specified multiple times.  Includes http/s_server_address by default.")
            ("http-threads", bpo::value<uint16_t>()->default_value( my->thread_pool_size ),
             "Number of worker threads in http thread pool")
            ("http-keep-alive", bpo::value<bool>()->default_value(true),
             "If set to false, do not keep HTTP connections alive, even if client requests.")
            ("http-ipv6", bpo::value<bool>()->default_value(false),
             "If set to true, bind http-server-address to IPv6 instead of IPv4.")             
            ;

   }


   void http_plugin::plugin_initialize(const variables_map& options) {
      try {
         my->plugin_state->validate_host = options.at("http-validate-host").as<bool>();
         if( options.count( "http-alias" )) {
            const auto& aliases = options["http-alias"].as<vector<string>>();
            for (auto &addr_str : aliases) {
               // get the host/port as a pair 
               auto host_port = get_host_port(addr_str, uri_default_port);
               my->plugin_state->valid_hosts.emplace(host_port);
            }
         }

         my->plugin_state->ipv6 = options.at("http-ipv6").as<bool>();
         auto server_protocol = tcp::v4();
         if (my->plugin_state->ipv6)
            server_protocol = tcp::v6();            

         tcp::resolver resolver( app().get_io_service());
         if( options.count( "http-server-address" ) && options.at( "http-server-address" ).as<string>().length()) {
            string lipstr = options.at( "http-server-address" ).as<string>();

            // get the host/port as a pair
            auto host_port = get_host_port(lipstr, uri_default_port);
            string port_str = std::to_string(host_port.second);
            
            try {
               my->listen_endpoint = *resolver.resolve( server_protocol, host_port.first, port_str);
               ilog( "configured http to listen on ${h}:${p}", ("h", host_port.first)( "p", host_port.second));
            } catch ( const boost::system::system_error& ec ) {
               elog( "failed to configure http to listen on ${h}:${p} (${m})",
                     ("h", host_port.first)( "p", host_port.second )( "m", ec.what()));
            }

            // add in resolved hosts and ports as well
            if (my->listen_endpoint) {
               my->add_aliases_for_endpoint(*my->listen_endpoint, host_port);
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

            // get the host/port as a pair 
            auto host_port = get_host_port(lipstr, uri_default_secure_port);
            string port_str = std::to_string(host_port.second);

            try {
               my->https_listen_endpoint = *resolver.resolve( server_protocol, host_port.first, port_str );
               ilog( "configured https to listen on ${h}:${p} (TLS configuration will be validated momentarily)",
                     ("h", host_port.first)( "p", host_port.second ));
               my->https_cert_chain = options.at( "https-certificate-chain-file" ).as<string>();
               my->https_key = options.at( "https-private-key-file" ).as<string>();
            } catch ( const boost::system::system_error& ec ) {
               elog( "failed to configure https to listen on ${h}:${p} (${m})",
                     ("h", host_port.first)( "p", host_port.second )( "m", ec.what()));
            }

            // add in resolved hosts and ports as well
            if (my->https_listen_endpoint) {
               my->add_aliases_for_endpoint(*my->https_listen_endpoint, host_port);
            }
         }

         my->plugin_state->max_body_size = options.at( "max-body-size" ).as<uint32_t>();
         verbose_http_errors = options.at( "verbose-http-errors" ).as<bool>();
         my->plugin_state->verbose_http_errors = verbose_http_errors;

         my->thread_pool_size = options.at( "http-threads" ).as<uint16_t>();
         EOS_ASSERT( my->thread_pool_size > 0, chain::plugin_config_exception,
                     "http-threads ${num} must be greater than 0", ("num", my->thread_pool_size));

         my->plugin_state->max_bytes_in_flight = options.at( "http-max-bytes-in-flight-mb" ).as<uint32_t>() * 1024 * 1024;
         my->plugin_state->max_requests_in_flight = options.at( "http-max-in-flight-requests" ).as<int32_t>();
         my->plugin_state->max_response_time = fc::microseconds( options.at("http-max-response-time-ms").as<uint32_t>() * 1000 );

         my->plugin_state->keep_alive = options.at("http-keep-alive").as<bool>();
         //watch out for the returns above when adding new code here
      } FC_LOG_AND_RETHROW()
   }

   void http_plugin::plugin_startup() {

      handle_sighup(); // setup logging
      app().post(appbase::priority::high, [this] ()
      {
         try {
            my->thread_pool = std::make_shared<eosio::chain::named_thread_pool>( "http", my->thread_pool_size );
            if(my->listen_endpoint) {
               try {
                  my->create_beast_server(false);

                  fc_ilog( logger, "start listening for http requests (boost::beast)" );

                  my->beast_server->listen(*my->listen_endpoint);
                  my->beast_server->start_accept();
               } catch ( const fc::exception& e ){
                  fc_elog( logger, "http service failed to start: ${e}", ("e", e.to_detail_string()) );
                  throw;
               } catch ( const std::exception& e ){
                  fc_elog( logger, "http service failed to start: ${e}", ("e", e.what()) );
                  throw;
               } catch (...) {
                  fc_elog( logger, "error thrown from http io service" );
                  throw;
               }
            }

            if(my->unix_endpoint) {
               try {
                  my->create_beast_server(false, true);
                  
                  my->beast_unix_server->listen(*my->unix_endpoint);
                  my->beast_unix_server->start_accept();
               } catch ( const fc::exception& e ){
                  fc_elog( logger, "unix socket service (${path}) failed to start: ${e}", ("e", e.to_detail_string())("path",my->unix_endpoint->path()) );
                  throw;
               } catch ( const std::exception& e ){
                  fc_elog( logger, "unix socket service (${path}) failed to start: ${e}", ("e", e.what())("path",my->unix_endpoint->path()) );
                  throw;
               } catch (...) {
                  fc_elog( logger, "error thrown from unix socket (${path}) io service", ("path",my->unix_endpoint->path()) );
                  throw;
               }
            }

            if(my->https_listen_endpoint) {
               try {
                  my->create_beast_server(true);

                  fc_ilog( logger, "start listening for https requests (boost::beast)" );
                  my->beast_https_server->listen(*my->https_listen_endpoint);
                  my->beast_https_server->start_accept();
               } catch ( const fc::exception& e ){
                  fc_elog( logger, "https service failed to start: ${e}", ("e", e.to_detail_string()) );
                  throw;
               } catch ( const std::exception& e ){
                  fc_elog( logger, "https service failed to start: ${e}", ("e", e.what()) );
                  throw;
               } catch (...) {
                  fc_elog( logger, "error thrown from https io service" );
                  throw;
               }
            }
            
            add_api({{
               std::string("/v1/node/get_supported_apis"),
               [&](const string&, string body, url_response_callback cb) mutable {
                  try {
                     if (body.empty()) body = "{}";
                     auto result = (*this).get_supported_apis();
                     cb(200, fc::variant(result));
                  } catch (...) {
                     handle_exception("node", "get_supported_apis", body, cb);
                  }
               }
            }});
            
         } catch (...) {
            fc_elog(logger, "http_plugin startup fails, shutting down");
            app().quit();
         }
      });
   }

   void http_plugin::handle_sighup() {
      fc::logger::update( logger_name, logger );
   }

   void http_plugin::plugin_shutdown() {
      if(my->beast_server)
         my->beast_server->stop_listening();
      if(my->beast_https_server)
         my->beast_https_server->stop_listening();
      if(my->beast_unix_server)
         my->beast_unix_server->stop_listening();

      if( my->thread_pool ) {
         my->thread_pool->stop();
         my->thread_pool.reset();
      }

      // release http_plugin_impl_ptr shared_ptrs captured in url handlers
      my->plugin_state->url_handlers.clear();

      app().post( 0, [me = my](){} ); // keep my pointer alive until queue is drained
   }

   void http_plugin::add_handler(const string& url, const url_handler& handler, int priority) {
      fc_ilog( logger, "add api url: ${c}", ("c", url) );
      my->plugin_state->url_handlers[url] = my->make_app_thread_url_handler(priority, handler, my);
   }

   void http_plugin::add_async_handler(const string& url, const url_handler& handler) {
      fc_ilog( logger, "add api url: ${c}", ("c", url) );
      my->plugin_state->url_handlers[url] = my->make_http_thread_url_handler(handler);
   }

   void http_plugin::handle_exception( const char *api_name, const char *call_name, const string& body, url_response_callback cb) {
      try {
         try {
            throw;
         } catch (chain::unknown_block_exception& e) {
            error_results results{400, "Unknown Block", error_results::error_info(e, verbose_http_errors)};
            cb( 400, fc::variant( results ));
         } catch (chain::invalid_http_request& e) {
            error_results results{400, "Invalid Request", error_results::error_info(e, verbose_http_errors)};
            cb( 400, fc::variant( results ));
         } catch (chain::unsatisfied_authorization& e) {
            error_results results{401, "UnAuthorized", error_results::error_info(e, verbose_http_errors)};
            cb( 401, fc::variant( results ));
         } catch (chain::tx_duplicate& e) {
            error_results results{409, "Conflict", error_results::error_info(e, verbose_http_errors)};
            cb( 409, fc::variant( results ));
         } catch (fc::eof_exception& e) {
            error_results results{422, "Unprocessable Entity", error_results::error_info(e, verbose_http_errors)};
            cb( 422, fc::variant( results ));
            fc_elog( logger, "Unable to parse arguments to ${api}.${call}", ("api", api_name)( "call", call_name ) );
            fc_dlog( logger, "Bad arguments: ${args}", ("args", body) );
         } catch (fc::exception& e) {
            error_results results{500, "Internal Service Error", error_results::error_info(e, verbose_http_errors)};
            cb( 500, fc::variant( results ));
            fc_dlog( logger, "Exception while processing ${api}.${call}: ${e}",
                     ("api", api_name)( "call", call_name )("e", e.to_detail_string()) );
         } catch (std::exception& e) {
            error_results results{500, "Internal Service Error", error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, e.what())), verbose_http_errors)};
            cb( 500, fc::variant( results ));
            fc_elog( logger, "STD Exception encountered while processing ${api}.${call}",
                     ("api", api_name)( "call", call_name ) );
            fc_dlog( logger, "Exception Details: ${e}", ("e", e.what()) );
         } catch (...) {
            error_results results{500, "Internal Service Error",
               error_results::error_info(fc::exception( FC_LOG_MESSAGE( error, "Unknown Exception" )), verbose_http_errors)};
            cb( 500, fc::variant( results ));
            fc_elog( logger, "Unknown Exception encountered while processing ${api}.${call}",
                     ("api", api_name)( "call", call_name ) );
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
      return my->plugin_state->verbose_http_errors;
   }

   http_plugin::get_supported_apis_result http_plugin::get_supported_apis()const {
      get_supported_apis_result result;

      for (const auto& handler : my->plugin_state->url_handlers) {
         if (handler.first != "/v1/node/get_supported_apis")
            result.apis.emplace_back(handler.first);
      }

      return result;
   }

   fc::microseconds http_plugin::get_max_response_time()const {
      return my->plugin_state->max_response_time;
   }

   std::istream& operator>>(std::istream& in, https_ecdh_curve_t& curve) {
      std::string s;
      in >> s;
      if (s == "secp384r1")
         curve = SECP384R1;
      else if (s == "prime256v1")
         curve = PRIME256V1;
      else
         in.setstate(std::ios_base::failbit);
      return in;
   }

   std::ostream& operator<<(std::ostream& osm, https_ecdh_curve_t curve) {
      if (curve == SECP384R1) {
         osm << "secp384r1";
      } else if (curve == PRIME256V1) {
         osm << "prime256v1";
      }

      return osm;
   }
}
