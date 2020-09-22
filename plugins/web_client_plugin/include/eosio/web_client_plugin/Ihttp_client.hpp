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
   virtual void exec(std::string_view host,
                     uint32_t port,
                     std::string_view method, 
                     std::string_view path,
                     client_handler callback,
                     std::string_view post_data,
                     const header_map* header) = 0;
};

struct Ihttps_client : Ihttp_client{
   virtual ~Ihttps_client(){};

   /**
    * @brief setup certificate for https requests
    * @param cert certificate in PEM format
    */
   virtual void init_ssl(std::string_view cert) = 0;
};

}}