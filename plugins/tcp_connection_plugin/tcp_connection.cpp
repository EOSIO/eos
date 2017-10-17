#include <eos/tcp_connection_plugin/tcp_connection.hpp>

#include <fc/log/logger.hpp>

#include <boost/asio.hpp>

namespace eos {

tcp_connection::tcp_connection(boost::asio::ip::tcp::socket s) :
   socket(std::move(s)),
   strand(socket.get_io_service())
{
   ilog("connection created");

   read();
}

tcp_connection::~tcp_connection() {
   ilog("Connection destroyed"); //XXX debug
}

void tcp_connection::read() {
   socket.async_read_some(boost::asio::buffer(rxbuffer), strand.wrap([this](auto ec, auto r) {
      read_ready(ec, r);
   }));
}

void tcp_connection::read_ready(boost::system::error_code ec, size_t red) {
   ilog("read ${r} bytes!", ("r",red));
   if(ec == boost::asio::error::eof) {
       socket.close();
       on_disconnected();
       return;
   }

   queuedOutgoing.emplace_front(rxbuffer, rxbuffer+red);

   ///XXX Note this is wrong Wrong WRONG as the async sends don't guarantee ordering;
   //     there needs to be a work queue of sorts.

   boost::asio::async_write(socket,
                            boost::asio::buffer(queuedOutgoing.front().data(), queuedOutgoing.front().size()),
                            strand.wrap([this, it=queuedOutgoing.begin()](auto ec, auto s) {
                               send_complete(ec, s, it);
                            }));

   read();
}

void tcp_connection::send_complete(boost::system::error_code ec, size_t sent, std::list<std::vector<uint8_t>>::iterator it) {
   if(ec) {
      socket.close();
      on_disconnected();
      return;
   }
   //sent should always == sent requested unless error, right? because _write() not _send_some()etc
   assert(sent == it->size());
   queuedOutgoing.erase(it);
}

bool tcp_connection::disconnected() {
   return socket.is_open();
}

connection tcp_connection::connect_on_disconnected(const signal<void()>::slot_type& slot) {
   return on_disconnected.connect(slot);
}

}