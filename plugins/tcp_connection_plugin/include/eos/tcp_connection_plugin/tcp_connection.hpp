#pragma once

#include <eos/network_plugin/connection_interface.hpp>

#include <boost/signals2.hpp>
#include <boost/asio.hpp>

namespace eos {

class tcp_connection : public connection_interface {
public:
   tcp_connection(boost::asio::ip::tcp::socket s);
   ~tcp_connection();
   
   bool disconnected() override;
   connection connect_on_disconnected(const signal<void()>::slot_type& slot) override;

private:
   void read();
   void read_ready(boost::system::error_code ec, size_t red);

   void send_complete(boost::system::error_code ec, size_t sent, std::list<std::vector<uint8_t>>::iterator it);

   boost::asio::ip::tcp::socket socket;
   boost::asio::io_service::strand strand;

   uint8_t rxbuffer[4096];
   std::list<std::vector<uint8_t>> queuedOutgoing;

   signal<void()> on_disconnected;

};

using tcp_connection_ptr = std::shared_ptr<tcp_connection>;

}