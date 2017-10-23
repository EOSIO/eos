#pragma once

#include <eos/network_plugin/connection_interface.hpp>

#include <boost/signals2.hpp>
#include <boost/asio.hpp>

#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/aes.hpp>

namespace eos {

class tcp_connection : public connection_interface {
   public:
      tcp_connection(boost::asio::ip::tcp::socket s);
      ~tcp_connection();
   
      bool disconnected() override;
      connection on_disconnected(const signal<void()>::slot_type& slot) override;

      //set a callback to be fired when the connection is "open for business" -- any
      // initialization is done and it's ready to pass transactions, block, etc.
      void set_cb_on_ready(std::function<void()> cb);

   private:
      void finish_key_exchange(boost::system::error_code ec, size_t red, fc::ecc::private_key priv_key);

      void read();
      void read_ready(boost::system::error_code ec, size_t red);

      void send_complete(boost::system::error_code ec, size_t sent, std::list<std::vector<uint8_t>>::iterator it);

      void handle_failure();

      boost::asio::ip::tcp::socket socket;
      boost::asio::io_service::strand strand;

      uint8_t rxbuffer[3000000];
      std::list<std::vector<uint8_t>> queuedOutgoing;

      signal<void()> on_disconnected_sig;
      bool disconnected_fired{false};    //be sure to only fire the signal once

      fc::aes_encoder    sending_aes_enc_ctx;
      fc::aes_decoder    receiving_aes_dec_ctx;
};

using tcp_connection_ptr = std::shared_ptr<tcp_connection>;

}