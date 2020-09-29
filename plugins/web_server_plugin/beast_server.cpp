#include <memory>
#include <memory>
#include <utility>

#include <fc/io/raw.hpp>
#include <eosio/web_server_plugin/beast_server.hpp>

using namespace eosio::web;
using namespace std;

beast_server::beast_server(){}
beast_server::~beast_server(){}

void beast_server::init(server_address&& address, boost::asio::io_context* ctx){

   if (ctx) {
      context.emplace(ctx);
   }
   else{
      init(forward<server_address>(address), 1);
   }
}
void beast_server::init(server_address&& address, uint8_t thread_pool_size){
   
   vector<char> buffer(fc::raw::pack_size(address));
   fc::datastream<char*> ds(buffer.data(), buffer.size());
   ds << address;

   thread_pool.emplace( string(buffer.data(), ds.tellp()), thread_pool_size );//TODO customize
}
void beast_server::add_handler(std::string_view path, server_handler callback){}
void beast_server::run(){}

void beast_server::init_ssl(std::string_view cert, std::string_view pk){}

beast_factory::beast_factory(){}
beast_factory::~beast_factory(){}

Ihttps_server* beast_factory::create_server(server_address&& address, boost::asio::io_context* context){
   unique_ptr<Ihttps_server> p_srv{ new beast_server() };
   p_srv->init(forward<server_address>(address), context);
   
   return p_srv.release();
}