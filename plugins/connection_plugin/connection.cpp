#include <eos/connection_plugin/connection.hpp>

#include <fc/log/logger.hpp>

#include <boost/asio.hpp>

namespace eos {

connection::connection(boost::asio::ip::tcp::socket s) :
   socket(std::move(s)),
   strand(socket.get_io_service())
{
   ilog("connection created");

   read();
}

connection::~connection() {
   ilog("Connection destroyed"); //XXX debug
}

void connection::read() {
   socket.async_read_some(boost::asio::buffer(rxbuffer), strand.wrap(boost::bind(&connection::read_ready, this, _1, _2)));
}

void connection::read_ready(boost::system::error_code ec, size_t red) {
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
                            strand.wrap(boost::bind(&connection::send_complete, this, _1, _2, queuedOutgoing.begin())));

   read();
}

void connection::send_complete(boost::system::error_code ec, size_t sent, std::list<std::vector<uint8_t>>::iterator it) {
   if(ec) {
      socket.close();
      on_disconnected();
      return;
   }
   //sent should always == sent requested unless error, right? because _write() not _send_some()etc
   assert(sent == it->size());
   queuedOutgoing.erase(it);
}

bool connection::disconnected() {
   return socket.is_open();
 }

 boost::signals2::connection connection::connect_on_disconnected(const boost::signals2::signal<void()>::slot_type& slot) {
   return on_disconnected.connect(slot);
 }

}