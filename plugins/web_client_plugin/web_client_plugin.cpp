#include <eosio/web_client_plugin/web_client_plugin.hpp>
#include <eosio/web_client_plugin/beast_client.hpp>

using namespace eosio;
using namespace eosio::web;

web_client_plugin::web_client_plugin()
   : impl(new beast_client{} ){
}
web_client_plugin::web_client_plugin(web::Ihttps_client* client)
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
void exec(web::schema_type schema,
          std::string_view host,
          uint32_t port,
          web::method_type method, 
          std::string_view path,
          web::client_handler callback,
          std::string_view post_data,
          const web::Ihttp_client::header_map* header){
   
}
web::authority web_client_plugin::auth(web::schema_type schema, const std::string& host, uint32_t port){
   return {schema, host, port, impl};
}