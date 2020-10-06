#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio/io_context.hpp>

#include <eosio/web_server_plugin/http_server_interface.hpp>
#include <fc/variant.hpp>

namespace eosio{
namespace web{

/**
 * @brief callback with response from server
 * @param response_code response code from server
 * @param last_chunk indicates if this is last chunk. 
 * single chunk response have this true
 * @param body body of current request
 * @param header server headers
 */
using client_handler = std::function<void(uint32_t response_code, 
                                          bool last_chunk, 
                                          std::string_view body,
                                          const header_interface& header)>;

struct http_session_interface{

   /**
    * @brief map for input client headers
    */
   using header_map = std::vector<std::pair<std::string, fc::variant>>;

   virtual ~http_session_interface(){}

   virtual void exec(method_type method, 
                     std::string_view path,
                     client_handler callback,
                     std::string_view post_data,
                     const header_map* header) = 0;
   virtual void close() = 0;
};

struct authority;

struct http_client_interface{
   using header_map = http_session_interface::header_map;
   using session_handler = std::function<void(http_session_interface* session)>;

   virtual ~http_client_interface(){};

   /**
    * @brief initialize client
    * @param context execution context to be used to server interaction 
    */
   virtual void init(boost::asio::io_context& context) = 0;
   /**
    * @brief execute http or https request
    * this implies Keep-Alive = false
    * @param host server host
    * @param port server port
    * @param method request method: GET, POST
    * @param callback handler that will be executed upon server response
    * @param post_data optional parameter for output data if executing POST method
    * @param header optional parameter with custom headers
    */
   virtual void exec(schema_type schema,
                     std::string_view host,
                     port_type port,
                     method_type method, 
                     std::string_view path,
                     client_handler callback,
                     std::string_view post_data,
                     const header_map* header) = 0;
   /**
    * @brief obtains http or https session.
    * this implies Keep-Alive = true
    */
   virtual http_session_interface* session(schema_type schema, const std::string& host, port_type port) = 0;
   /**
    * @brief  obtains http or https session asynchronously
    * this implies Keep-Alive = true
    */
   virtual void session_async(schema_type schema, const std::string& host, port_type port, session_handler callback) = 0;

   /**
    * @brief convenience shortcut. 
    * client.auth(schema, host, port) is equal to:
    * authority{schema, host, port, client_ptr}
    * TODO: think if we are really need this in our interface
    */
   virtual authority auth(schema_type schema, const std::string& host, port_type port) = 0;
};

struct https_client_interface : http_client_interface{
   virtual ~https_client_interface(){};

   /**
    * @brief setup certificate for https requests
    * @param cert certificate in PEM format
    */
   virtual void init_ssl(std::string_view cert) = 0;
};

using http_session_ptr = std::shared_ptr<http_session_interface>;
using http_client_ptr = std::shared_ptr<http_client_interface>;

struct url_w_handler {
   schema_type schema;
   std::string host;
   port_type   port;
   method_type method;
   std::string path;
   http_client_interface::header_map headers;
   client_handler handler;
   http_client_ptr client_ptr;   /* unused if session_ptr is set */
   http_session_ptr session_ptr; /* unused if client_ptr is set */

   inline void exec(){
      exec({});
   }
   inline void exec(std::string_view post_data){
      if (session_ptr) {
         EOS_ASSERT(!client_ptr, chain::other_http_exception, "client_ptr must be NULL if session_ptr is set");
         session_ptr->exec(method, path, handler, {}, &headers);
      }
      else {
         client_ptr->exec(schema, host, port, method, path, handler, post_data, &headers);
      }
   }
};

struct url_w_headers {
   schema_type schema;
   std::string host;
   port_type   port;
   method_type method;
   std::string path;
   http_client_interface::header_map headers;
   http_client_ptr client_ptr;   /* unused if session_ptr is set */
   http_session_ptr session_ptr; /* unused if client_ptr is set */

   inline void exec(client_handler callback){
      exec(callback, {});
   }
   inline void exec(client_handler callback, std::string_view post_data){
      if (session_ptr) {
         EOS_ASSERT(!client_ptr, chain::other_http_exception, "client_ptr must be NULL if session_ptr is set");
         session_ptr->exec(method, path, callback, post_data, &headers);
      }
      else {
         client_ptr->exec(schema, host, port, method, path, callback, post_data, &headers);
      }
   }
   inline url_w_handler handler(client_handler callback) &{
      return {schema, host, port, method, path, headers, callback, client_ptr};
   }
   inline url_w_handler handler(client_handler callback) &&{
      return {schema, std::move(host), port, method, std::move(path), std::move(headers), callback, std::move(client_ptr)};
   }
};

struct url {
   using header_map = http_client_interface::header_map;

   schema_type schema;     
   std::string host;       
   port_type   port;       
   method_type method;
   std::string path;
   http_client_ptr client_ptr;   /* unused if session_ptr is set */
   http_session_ptr session_ptr; /* unused if client_ptr is set */

   inline void exec(client_handler callback, std::string_view post_data){
      if (session_ptr) {
         EOS_ASSERT(!client_ptr, chain::other_http_exception, "client_ptr must be NULL if session_ptr is set");
         session_ptr->exec(method, path, callback, post_data, NULL);
      }
      else {
         client_ptr->exec(schema, host, port, method, path, callback, post_data, NULL);
      }
   }
   inline void exec(client_handler callback){
      exec(callback, {});
   }
   inline url_w_headers headers(header_map&& headers) &{
      return {schema, host, port, method, path, std::move(headers), client_ptr};
   }
   inline url_w_headers headers(header_map&& headers) &&{
      return {schema, std::move(host), port, method, std::move(path), std::move(headers), std::move(client_ptr)};
   }
   inline url_w_headers headers(header_map& headers) &{
      return {schema, host, port, method, path, headers, client_ptr};
   }
   inline url_w_headers headers(header_map& headers) &&{
      return {schema, std::move(host), port, method, std::move(path), headers, std::move(client_ptr)};
   }

   inline url_w_handler handler(client_handler callback) &{
      return {schema, host, port, method, path, {}, callback, client_ptr};
   }
   inline url_w_handler handler(client_handler callback) &&{
      return {schema, std::move(host), port, method, std::move(path), {}, callback, std::move(client_ptr)};
   }
};

struct session {
   schema_type schema;
   std::string host;
   port_type   port;
   http_session_ptr session_ptr;

   inline struct url path(std::string&& path, method_type method) const &{
      return {schema, host, port, method, std::move(path), {}, session_ptr};
   }
   inline struct url path(std::string&& path) const &{
      return session::path(std::forward<std::string>(path), method_type::GET);
   }
   inline struct url path(std::string&& path, method_type method) &&{
      return {std::move(schema), 
              std::move(host), 
              std::move(port), 
              std::move(method), 
              std::move(path), 
              {},
              std::move(session_ptr)};
   }
   inline struct url path(std::string&& path) &&{
      return session::path(std::forward<std::string>(path), method_type::GET);
   }
   inline void close(){
      session_ptr->close();
      session_ptr.reset();//we don't need closed session
   }
};

struct authority {
   using session_handler = std::function<void(session&&)>;

   schema_type schema;
   std::string host;
   port_type   port;
   std::shared_ptr<http_client_interface> client;
   
   void exec(method_type method, std::string_view path, client_handler callback) const{
      client->exec(schema, host, port, method, path, callback, {}, NULL);
   }
   inline struct url path(std::string&& path, method_type method) const &{
      return {schema, host, port, method, std::move(path), client, {}};
   }
   inline struct url path(std::string&& path) const &{
      return authority::path(std::forward<std::string>(path), method_type::GET);
   }
   inline struct url path(std::string&& path, method_type method) &&{
      return {std::move(schema), 
              std::move(host), 
              std::move(port), 
              std::move(method), 
              std::move(path), 
              std::move(client),
              {}};
   }
   inline struct url path(std::string&& path) &&{
      return authority::path(std::forward<std::string>(path), method_type::GET);
   }
   inline struct session session() const &{
      return { schema, host, port, http_session_ptr(client->session(schema, host, port)) };
   }
   inline struct session session() const &&{
      return { std::move(schema), std::move(host), std::move(port), http_session_ptr(client->session(schema, host, port)) };
   }
   inline void session_async(session_handler callback) const{
      client->session_async(schema, 
                            host, 
                            port, 
                            [&](http_session_interface* sess) { 
                               callback( {schema, host, port, http_session_ptr(sess)} );
                            });
   }
};

}}