#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/asio/io_context.hpp>

#include <eosio/web_server_plugin/Ihttp_server.hpp>
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
                                          const IHeader& header)>;

enum class schema_type : uint8_t {
   HTTP,
   HTTPS
};

enum class method_type : uint8_t {
   GET,
   POST
};

struct authority;
struct url;
struct url_w_headers;

struct Ihttp_client{
   /**
    * @brief map for input client headers
    */
   using header_map = std::vector<std::pair<std::string, fc::variant>>;

   virtual ~Ihttp_client(){};

   /**
    * @brief initialize client
    * @param context execution context to be used to server interaction 
    */
   virtual void init(boost::asio::io_context& context) = 0;
   /**
    * @brief execute http or https request
    * @param host server host
    * @param port server port
    * @param method request method: GET, POST
    * @param callback handler that will be executed upon server response
    * @param post_data optional parameter for output data if executing POST method
    * @param header optional parameter with custom headers
    */
   virtual void exec(schema_type schema,
                     std::string_view host,
                     uint32_t port,
                     method_type method, 
                     std::string_view path,
                     client_handler callback,
                     std::string_view post_data,
                     const header_map* header) = 0;
   virtual authority auth(schema_type schema,
                          const std::string& host,
                          uint32_t port) = 0;
};

struct Ihttps_client : Ihttp_client{
   virtual ~Ihttps_client(){};

   /**
    * @brief setup certificate for https requests
    * @param cert certificate in PEM format
    */
   virtual void init_ssl(std::string_view cert) = 0;
};

struct url_w_handler {
   schema_type schema;
   std::string host;
   uint32_t    port;
   method_type method;
   std::string path;
   Ihttp_client::header_map headers;
   client_handler handler;
   std::shared_ptr<Ihttp_client> client;

   void exec(){
      client->exec(schema, host, port, method, path, handler, {}, &headers);
   }
   void post(std::string_view post_data){
      client->exec(schema, host, port, method, path, handler, post_data, &headers);
   }
};

struct url_w_headers {
   schema_type schema;
   std::string host;
   uint32_t    port;
   method_type method;
   std::string path;
   Ihttp_client::header_map headers;
   std::shared_ptr<Ihttp_client> client;

   void exec(client_handler callback){
      client->exec(schema, host, port, method, path, callback, {}, &headers);
   }
   void exec(client_handler callback, std::string_view post_data){
      client->exec(schema, host, port, method, path, callback, post_data, &headers);
   }
   url_w_handler handler(client_handler callback) &{
      return {schema, host, port, method, path, headers, callback, client};
   }
   url_w_handler handler(client_handler callback) &&{
      return {schema, std::move(host), port, method, std::move(path), std::move(headers), callback, std::move(client)};
   }
};

struct url {
   schema_type schema;
   std::string host;
   uint32_t    port;
   method_type method;
   std::string path;
   std::shared_ptr<Ihttp_client> client;

   void exec(client_handler callback){
      client->exec(schema, host, port, method, path, callback, {}, NULL);
   }
   void exec(client_handler callback, std::string_view post_data){
      client->exec(schema, host, port, method, path, callback, post_data, NULL);
   }

   url_w_headers headers(Ihttp_client::header_map&& headers) &{
      return {schema, host, port, method, path, std::move(headers), client};
   }
   url_w_headers headers(Ihttp_client::header_map&& headers) &&{
      return {schema, std::move(host), port, method, std::move(path), std::move(headers), std::move(client)};
   }
   url_w_headers headers(Ihttp_client::header_map& headers) &{
      return {schema, host, port, method, path, headers, client};
   }
   url_w_headers headers(Ihttp_client::header_map& headers) &&{
      return {schema, std::move(host), port, method, std::move(path), headers, std::move(client)};
   }
   url_w_handler handler(client_handler callback) &{
      return {schema, host, port, method, path, {}, callback, client};
   }
   url_w_handler handler(client_handler callback) &&{
      return {schema, std::move(host), port, method, std::move(path), {}, callback, std::move(client)};
   }
};

struct authority {
   schema_type schema;
   std::string host;
   uint32_t    port;
   std::shared_ptr<Ihttp_client> client;
   
   void exec(method_type method, std::string_view path, client_handler callback){
      client->exec(schema, host, port, method, path, callback, {}, NULL);
   }
   inline struct url url(const std::string& path) &{
      return {schema, host, port, method_type::GET, path, client};
   }
   inline struct url url(std::string&& path) &&{
      return {schema, std::move(host), port, method_type::GET, std::move(path), std::move(client)};
   }
   inline struct url url(method_type method, const std::string& path) &{
      return {schema, host, port, method, path, client};
   }
   inline struct url url(method_type method, std::string&& path) &{
      return {schema, host, port, method, std::move(path), client};
   }
   inline struct url url(method_type method, const std::string& path) &&{
      return {std::move(schema), 
              std::move(host), 
              std::move(port), 
              method, 
              path, 
              std::move(client)};
   }
   inline struct url url(method_type method, std::string&& path) &&{
      return {std::move(schema), 
              std::move(host), 
              std::move(port), 
              method, 
              std::move(path), 
              std::move(client)};
   }
};

}}