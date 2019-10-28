#pragma once

#include <string>
#include <vector>
#include <fc/variant.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>
#include <fc/static_variant.hpp>

namespace eosio { namespace client { namespace http {

   using namespace std;

   namespace detail {
      class http_context_impl;

      struct http_context_deleter {
         void operator()(http_context_impl*) const;
      };
   }

   using http_context = std::unique_ptr<detail::http_context_impl, detail::http_context_deleter>;

   http_context create_http_context();

   struct parsed_url {
      string scheme;
      string server;
      string port;
      string path;

      static string normalize_path(const string& path);

      parsed_url operator+ (string sub_path) {
         return {scheme, server, port, path + sub_path};
      }
   };

   parsed_url parse_url( const string& server_url );

   struct resolved_url : parsed_url {
      resolved_url( const parsed_url& url, vector<string>&& resolved_addresses, uint16_t resolved_port, bool is_loopback)
      :parsed_url(url)
      ,resolved_addresses(std::move(resolved_addresses))
      ,resolved_port(resolved_port)
      ,is_loopback(is_loopback)
      {
      }

      //used for unix domain, where resolving and ports are nonapplicable
      resolved_url(const parsed_url& url) : parsed_url(url) {}

      vector<string> resolved_addresses;
      uint16_t resolved_port = 0;
      bool is_loopback = false;
   };

   resolved_url resolve_url( const http_context& context,
                             const parsed_url& url );

   struct connection_param {
      const http_context& context;
      resolved_url url;
      bool verify_cert;
      std::vector<string>& headers;

      connection_param( const http_context& context,
                        const resolved_url& url,
                        bool verify,
                        std::vector<string>& h) : context(context),url(url), headers(h) {
         verify_cert = verify;
      }

      connection_param( const http_context& context,
                        const parsed_url& url,
                        bool verify,
                        std::vector<string>& h) : context(context),url(resolve_url(context, url)), headers(h) {
         verify_cert = verify;
      }
   };

   fc::variant do_http_call( const connection_param& cp,
                             const fc::variant& postdata = fc::variant(),
                             bool print_request = false,
                             bool print_response = false);

   fc::variant do_http_call( const connection_param& cp,
                             const std::string &postjson,
                             bool print_request = false,
                             bool print_response = false );
 }}}
