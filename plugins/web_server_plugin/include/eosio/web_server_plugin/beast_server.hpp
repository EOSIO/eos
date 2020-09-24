#pragma once
#include <eosio/web_server_plugin/Ihttp_server.hpp>


namespace eosio{
namespace web{

struct beast_server : Ihttps_server{
   beast_server();
   virtual ~beast_server();

   //IhttpServer:
   virtual void init(boost::asio::io_context& context, std::string_view host, uint32_t port);
   virtual void add_handler(std::string_view path, server_handler callback);
   virtual void run();

   //Ihttps_server:
   //use PEM format for certificate and pk
   virtual void init_ssl(std::string_view cert, std::string_view pk);
};

}}