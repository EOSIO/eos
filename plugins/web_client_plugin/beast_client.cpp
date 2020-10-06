#include <eosio/web_client_plugin/beast_client.hpp>

using namespace eosio;
using namespace eosio::web;

beast_client::beast_client(){}
beast_client::~beast_client(){}

void beast_client::init(boost::asio::io_context& context){
}

void beast_client::exec(schema_type schema,
                  std::string_view host,
                  port_type port,
                  method_type method, 
                  std::string_view path,
                  client_handler callback,
                  std::string_view post_data,
                  const header_map* header){
   
}
authority beast_client::auth(schema_type schema, const std::string& host, port_type port){
   return {schema, host, port, shared_from_this()};
}
http_session_interface* beast_client::session(schema_type schema, const std::string& host, port_type port){
   return NULL;
}
void beast_client::session_async(schema_type schema, 
                                           const std::string& host, 
                                           port_type port, 
                                           session_handler callback){
   
}
void beast_client::init_ssl(std::string_view cert){
}