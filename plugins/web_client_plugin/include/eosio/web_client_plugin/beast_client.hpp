#pragma once

#include <eosio/web_client_plugin/Ihttp_client.hpp>

namespace eosio{
namespace web{

struct beast_client : Ihttps_client{
   beast_client();
   virtual ~beast_client();

   //Ihttp_client:
   virtual void init(boost::asio::io_context& context);
   virtual void exec(std::string_view host,
                     uint32_t port,
                     std::string_view method, 
                     std::string_view path,
                     client_handler callback,
                     std::string_view post_data,
                     const Ihttps_client::header_map* header);

   //Ihttps_client:
   //use PEM format for certificate
   virtual void init_ssl(std::string_view cert);
};

}}