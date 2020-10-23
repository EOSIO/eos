#include <unordered_map>

#include <fc/exception/exception.hpp>

#include <eosio/web_server_plugin/web_server_plugin.hpp>
#include <eosio/web_server_plugin/beast_server.hpp>

using namespace std;
using namespace eosio;
using namespace eosio::web;
namespace asio = boost::asio;

namespace eosio{

class web_server_plugin_impl{
public:
   std::unique_ptr<http_server_generic_factory_interface> server_factory;

   struct server_address_cmp{
      size_t operator() (const server_address& lhs, const server_address& rhs) const {
         if (lhs.schema == rhs.schema){
            if (lhs.host == rhs.host){
               return lhs.port < rhs.port;
            }

            return lhs.host < rhs.host;
         }

         return lhs.schema < rhs.schema;
      }
   };

   web_server_plugin_impl() : server_factory(new beastimpl::web_server_factory{}) {}
   web_server_plugin_impl(http_server_generic_factory_interface* factory) : server_factory(factory) {}
   ~web_server_plugin_impl(){}

   http_server_generic_interface_ptr get_server(server_address&& address, asio::io_context* context){
      auto it = server_list.find(address);
      if (it != server_list.end()){
         return it->second;
      }

      //call to server_address(address) supposed to create copy of address object
      //this is needed because of we need it two times
      http_server_generic_interface_ptr p_server( server_factory->create_server(server_address{address}, context) );
      auto pair = make_pair(forward<server_address>(address), p_server);
      server_list.emplace( move(pair) );
      
      return p_server;
   }

private:
   map<server_address, std::shared_ptr<http_server_interface>, server_address_cmp> server_list; 
};

web_server_plugin::web_server_plugin() 
   : impl(new web_server_plugin_impl{}){
}
web_server_plugin::web_server_plugin(http_server_generic_factory_interface* factory)
   : impl(new web_server_plugin_impl(factory)){
}
web_server_plugin::~web_server_plugin(){
}

void web_server_plugin::set_program_options(options_description&, options_description& cfg){
   cfg.add_options()
      ("https-certificate-chain-file", bpo::value<string>(),
       "Filename with the certificate chain to present on https connections. PEM format. Required for https.")

      ("https-private-key-file", bpo::value<string>(),
       "Filename with https private key in PEM format. Required for https")

      ("https-ecdh-curve", bpo::value<bool>(),
       "Configure https ECDH curve to use: secp384r1 or prime256v1")
      
      ("server-address", bpo::value<string>(),
       "The local IP to listen for incoming http/https connections; leave blank to use localhost.")
      
      ("server-port", bpo::value<uint32_t>(),
       "The local port to listen for incoming http/https connections.");
}


void web_server_plugin::plugin_initialize(const variables_map& options){
   try {
      
   } FC_LOG_AND_RETHROW()
}

void web_server_plugin::plugin_startup(){
   handle_sighup();
}

void web_server_plugin::plugin_shutdown(){

}

void web_server_plugin::handle_sighup(){
   fc::logger::update( "web_server_plugin", logger );
}

void web_server_plugin::add_handler(std::string_view path, web::server_handler callback){
   //impl->add_handler(path, callback);
}

http_server_generic_interface_ptr web_server_plugin::get_server(server_address&& name, asio::io_context* context){
   return impl->get_server(forward<server_address&&>(name), context);
}

}