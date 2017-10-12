#pragma once

#include <eos/network_plugin/connection_interface.hpp>

#include <boost/signals2.hpp>
#include <boost/asio.hpp>

namespace eos {

class connection : public connection_interface {
public:
   connection(boost::asio::ip::tcp::socket s);
   ~connection();
   
   bool disconnected() override;
   boost::signals2::connection connect_on_disconnected(const boost::signals2::signal<void()>::slot_type& slot) override;

private:
   void read();
   void read_ready(boost::system::error_code ec, size_t red);

   void send_complete(boost::system::error_code ec, size_t sent, std::list<std::vector<uint8_t>>::iterator it);

   boost::asio::ip::tcp::socket socket;
   boost::asio::io_service::strand strand;

   uint8_t rxbuffer[4096];
   std::list<std::vector<uint8_t>> queuedOutgoing;

   boost::signals2::signal<void()> on_disconnected;

};

}