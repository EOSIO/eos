//
// sync_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <regex>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include "httpc.hpp"

using boost::asio::ip::tcp;
namespace eosio { namespace client { namespace http {
   void do_connect(tcp::socket& sock, const std::string& server, const std::string& port) {
      // Get a list of endpoints corresponding to the server name.
      tcp::resolver resolver(sock.get_io_service());
      tcp::resolver::query query(server, port);
      boost::asio::connect(sock, resolver.resolve(query));
   }

   template<class T>
   std::string do_txrx(T& socket, boost::asio::streambuf& request_buff, unsigned int& status_code) {
      // Send the request.
      boost::asio::write(socket, request_buff);

      // Read the response status line. The response streambuf will automatically
      // grow to accommodate the entire line. The growth may be limited by passing
      // a maximum size to the streambuf constructor.
      boost::asio::streambuf response;
      boost::asio::read_until(socket, response, "\r\n");

      // Check that response is OK.
      std::istream response_stream(&response);
      std::string http_version;
      response_stream >> http_version;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      FC_ASSERT( !(!response_stream || http_version.substr(0, 5) != "HTTP/"), "Invalid Response" );

      // Read the response headers, which are terminated by a blank line.
      boost::asio::read_until(socket, response, "\r\n\r\n");

      // Process the response headers.
      std::string header;
      int response_content_length = -1;
      std::regex clregex(R"xx(^content-length:\s+(\d+))xx", std::regex_constants::icase);
      while (std::getline(response_stream, header) && header != "\r") {
         std::smatch match;
         if(std::regex_search(header, match, clregex))
            response_content_length = std::stoi(match[1]);
      }
      FC_ASSERT(response_content_length >= 0, "Invalid content-length response");

      std::stringstream re;
      // Write whatever content we already have to output.
      response_content_length -= response.size();
      if (response.size() > 0)
         re << &response;

      boost::asio::read(socket, response, boost::asio::transfer_exactly(response_content_length));
      re << &response;

      return re.str();
   }

   parsed_url parse_url( const string& server_url ) {
      parsed_url res;

      //via rfc3986 and modified a bit to suck out the port number
      //Sadly this doesn't work for ipv6 addresses
      std::regex rgx(R"xx(^(([^:/?#]+):)?(//([^:/?#]*)(:(\d+))?)?([^?#]*)(\?([^#]*))?(#(.*))?)xx");
      std::smatch match;
      if(std::regex_search(server_url.begin(), server_url.end(), match, rgx)) {
         res.scheme = match[2];
         res.server = match[4];
         res.port = match[6];
         res.path_prefix = match[7];
      }
      if(res.scheme != "http" && res.scheme != "https")
         FC_THROW("Unrecognized URL scheme (${s}) in URL \"${u}\"", ("s", res.scheme)("u", server_url));
      if(res.server.empty())
         FC_THROW("No server parsed from URL \"${u}\"", ("u", server_url));
      if(res.port.empty())
         res.port = res.scheme == "http" ? "80" : "443";
      boost::trim_right_if(res.path_prefix, boost::is_any_of("/"));
      return res;
   }

   fc::variant do_http_call( const connection_param& cp,
                             const fc::variant& postdata,
                             bool print_request ) {
   std::string postjson;
   if( !postdata.is_null() ) {
      postjson = print_request ? fc::json::to_pretty_string( postdata ) : fc::json::to_string( postdata );
   }

   boost::asio::io_service io_service;

   const string& server_url = cp.url;
   const string& path = cp.path;

   auto url = parse_url( server_url );

   boost::asio::streambuf request;
   std::ostream request_stream(&request);
   request_stream << "POST " << url.path_prefix + path << " HTTP/1.0\r\n";
   request_stream << "Host: " << url.server << "\r\n";
   request_stream << "content-length: " << postjson.size() << "\r\n";
   request_stream << "Accept: */*\r\n";
   request_stream << "Connection: close\r\n";
   request_stream << "\r\n";
   // append more customized headers
   std::vector<string>::iterator itr;
   for (itr = cp.headers.begin(); itr != cp.headers.end(); itr++) {
      request_stream << *itr << "\r\n";
   }
   request_stream << postjson;

   if ( print_request ) {
      string s(request.size(), '\0');
      buffer_copy(boost::asio::buffer(s), request.data());
      std::cerr << "REQUEST:" << std::endl
                << "---------------------" << std::endl
                << s << std::endl
                << "---------------------" << std::endl;
   }

   unsigned int status_code;
   std::string re;

   if(url.scheme == "http") {
      tcp::socket socket(io_service);
      do_connect(socket, url.server, url.port);
      re = do_txrx(socket, request, status_code);
   }
   else { //https
      boost::asio::ssl::context ssl_context(boost::asio::ssl::context::sslv23_client);
#if defined( __APPLE__ )
      //TODO: this is undocumented/not supported; fix with keychain based approach
      ssl_context.load_verify_file("/private/etc/ssl/cert.pem");
#elif defined( _WIN32 )
      FC_THROW("HTTPS on Windows not supported");
#else
      ssl_context.set_default_verify_paths();
#endif

      boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket(io_service, ssl_context);
      if(cp.verify_cert)
         socket.set_verify_mode(boost::asio::ssl::verify_peer);

      do_connect(socket.next_layer(), url.server, url.port);
      socket.handshake(boost::asio::ssl::stream_base::client);
      re = do_txrx(socket, request, status_code);
      //try and do a clean shutdown; but swallow if this fails (other side could have already gave TCP the ax)
      try {socket.shutdown();} catch(...) {}
   }

   const auto response_result = fc::json::from_string(re);
   if( status_code == 200 || status_code == 201 || status_code == 202 ) {
      return response_result;
   } else if( status_code == 404 ) {
      // Unknown endpoint
      if (path.compare(0, chain_func_base.size(), chain_func_base) == 0) {
         throw chain::missing_chain_api_plugin_exception(FC_LOG_MESSAGE(error, "Chain API plugin is not enabled"));
      } else if (path.compare(0, wallet_func_base.size(), wallet_func_base) == 0) {
         throw chain::missing_wallet_api_plugin_exception(FC_LOG_MESSAGE(error, "Wallet is not available"));
      } else if (path.compare(0, account_history_func_base.size(), account_history_func_base) == 0) {
         throw chain::missing_history_api_plugin_exception(FC_LOG_MESSAGE(error, "History API plugin is not enabled"));
      } else if (path.compare(0, net_func_base.size(), net_func_base) == 0) {
         throw chain::missing_net_api_plugin_exception(FC_LOG_MESSAGE(error, "Net API plugin is not enabled"));
      }
   } else {
      auto &&error_info = response_result.as<eosio::error_results>().error;
      // Construct fc exception from error
      const auto &error_details = error_info.details;

      fc::log_messages logs;
      for (auto itr = error_details.begin(); itr != error_details.end(); itr++) {
         const auto& context = fc::log_context(fc::log_level::error, itr->file.data(), itr->line_number, itr->method.data());
         logs.emplace_back(fc::log_message(context, itr->message));
      }

      throw fc::exception(logs, error_info.code, error_info.name, error_info.what);
   }

   FC_ASSERT( status_code == 200, "Error code ${c}\n: ${msg}\n", ("c", status_code)("msg", re) );
   return response_result;
   }
}}}
