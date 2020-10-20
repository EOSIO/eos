#include <eosio/web_server_plugin/beast_server.hpp>

using namespace eosio::web;

beast_server::beast_server(){}
beast_server::~beast_server(){}

void beast_server::init(boost::asio::io_context& context, std::string_view host, uint32_t port){}
void beast_server::add_handler(std::string_view path, server_handler callback){}
void beast_server::run(){}

void beast_server::init_ssl(std::string_view cert, std::string_view pk){}