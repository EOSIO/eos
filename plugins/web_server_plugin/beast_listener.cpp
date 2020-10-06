#include <eosio/web_server_plugin/beast_listener.hpp>       /* http_listener/https_listener */
#include <eosio/chain/exceptions.hpp>                       /* tcp_accept_exception */

using namespace eosio::web::beastimpl;
using namespace boost::asio = asio;
using namespace beast = boost::beast;
using namespace std;

http_listener::http_listener(const endpoint& ep, io_context& ctx, const handler_map& callbacks)
   : address(ep), context(ctx), acceptor(asio::make_strand(ctx)), handlers(callbacks){
}

void http_listener::listen(){
   
   beast::error_code ec;

   acceptor.open(address.protocol(), ec);
   EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.open failed with error: {ec}", ("ec", ec));

   // Allow address reuse
   //this is to avoid 'address in use' error
   //explanation and how it works for different platforms you can read there:
   //https://stackoverflow.com/questions/14388706/how-do-so-reuseaddr-and-so-reuseport-differ
   acceptor.set_option(asio::socket_base::reuse_address(true), ec);
   EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.set_option failed with error: {ec}", ("ec", ec));
   
   acceptor.bind(address, ec);
   EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.bind failed with error: {ec}", ("ec", ec));

   acceptor.listen(asio::socket_base::max_listen_connections, ec);
   EOS_ASSERT(!ec, tcp_accept_exception, "acceptor.listen failed with error: {ec}", ("ec", ec));
}

void start_accept(accept_handler acc_handler){
   acceptor.async_accept(asio::make_strand(context), acc_handler);
}

void http_listener::start_accept(){
   
   //auto handler = beast::bind_front_handler(&http_listener::on_accept, shared_from_this());
   auto pthis = shared_from_this();
   start_accept([pthis](beast::error_code ec, tcp::socket s){
                  pthis->on_accept(ec, move(s));
                });
}
void http_listener::on_accept(boost::beast::error_code ec, socket s){
   //create session here and return pointer to caller
}
https_listener::https_listener(const endpoint& ep, io_context& ctx, const handler_map& callbacks, ssl_context& ssl_context)
   : http_listener(ep, ctx, callbacks), ssl_ctx(ssl_context){
}
void https_listener::start_accept(){
   auto pthis = shared_from_this();
   start_accept([pthis](beast::error_code ec, tcp::socket s){
                  pthis->on_accept(ec, move(s));
                });
}
void https_listener::on_accept(boost::beast::error_code ec, socket s){
   //create session here and return pointer to caller
}