#include <fc/exception/exception.hpp>

#include <eosio/web_server_plugin/web_server_plugin.hpp>
#include <eosio/web_server_plugin/beast_server.hpp>

using namespace eosio;
using namespace eosio::web;

web_server_plugin::web_server_plugin() 
   : impl(new beast_server{} ){
}
web_server_plugin::web_server_plugin(Ihttps_server* server)
   : impl(server){
}
web_server_plugin::~web_server_plugin(){
}

void web_server_plugin::set_program_options(options_description&, options_description& cfg) {
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


void web_server_plugin::plugin_initialize(const variables_map& options) {
   try {
      
   } FC_LOG_AND_RETHROW()
}

void web_server_plugin::plugin_startup() {

}

void web_server_plugin::plugin_shutdown() {

}

void web_server_plugin::add_handler(std::string_view path, web::server_handler callback){
   impl->add_handler(path, callback);
}
