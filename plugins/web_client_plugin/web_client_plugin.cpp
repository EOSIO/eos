#include <eosio/web_client_plugin/web_client_plugin.hpp>
#include <eosio/web_client_plugin/beast_client.hpp>

using namespace eosio;
using namespace eosio::web;

web_client_plugin::web_client_plugin()
   : impl(new beast_client{} ){
}
web_client_plugin::web_client_plugin(web::https_client_interface* client)
   : impl(client){
}

web_client_plugin::~web_client_plugin(){}

void web_client_plugin::set_program_options(options_description&, options_description& cfg){
}

void web_client_plugin::plugin_initialize(const variables_map& options){
}
void web_client_plugin::plugin_startup(){
}
void web_client_plugin::plugin_shutdown(){
}
void exec(schema_type schema,
          std::string_view host,
          port_type port,
          method_type method, 
          std::string_view path,
          client_handler callback,
          std::string_view post_data,
          const web::http_client_interface::header_map* header){
   
}
authority web_client_plugin::auth(schema_type schema, const std::string& host, port_type port){
   return {schema, host, port, impl};
}