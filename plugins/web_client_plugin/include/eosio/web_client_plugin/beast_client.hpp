#pragma once

#include <eosio/web_client_plugin/http_client_interface.hpp>

namespace eosio{
namespace web{

struct beast_client : https_client_interface, std::enable_shared_from_this<beast_client>{
   beast_client();
   virtual ~beast_client();

   //<http_client_interface>
   virtual void init(boost::asio::io_context& context);
   virtual void exec(schema_type schema,
                     std::string_view host,
                     port_type port,
                     method_type method, 
                     std::string_view path,
                     client_handler callback,
                     std::string_view post_data,
                     const header_map* header);
   virtual authority auth(schema_type schema,
                          const std::string& host,
                          port_type port);
   virtual http_session_interface* session(schema_type schema, const std::string& host, port_type port);
   virtual void session_async(schema_type schema, const std::string& host, port_type port, session_handler callback);
   //</http_client_interface>

   //<https_client_interface>
   //use PEM format for certificate
   virtual void init_ssl(std::string_view cert);
   //</https_client_interface>
};

}}