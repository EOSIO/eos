#pragma once
#include <eosio/web_server_plugin/Ihttp_server.hpp>
#include <eosio/chain/thread_utils.hpp>

namespace eosio{
namespace web{

struct beast_server : Ihttps_server{
   beast_server();
   virtual ~beast_server();

   //IhttpServer:
   virtual void init(server_address&& address, boost::asio::io_context* context);
   virtual void init(server_address&& address, uint8_t thread_pool_size);
   virtual void add_handler(std::string_view path, server_handler callback);
   virtual void run();

   //Ihttps_server:
   //use PEM format for certificate and pk
   virtual void init_ssl(std::string_view cert, std::string_view pk);

   std::optional<boost::asio::io_context*> context;
   std::optional<eosio::chain::named_thread_pool> thread_pool;
};

struct beast_factory : Ihttps_server_factory{
   beast_factory();
   virtual ~beast_factory();

   //caller is responsible for deletion of server object
   virtual Ihttps_server* create_server(server_address&& address, boost::asio::io_context* context);
};

}}