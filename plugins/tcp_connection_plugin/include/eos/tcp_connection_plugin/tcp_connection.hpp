#pragma once

#include <eos/network_plugin/connection_interface.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>

#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/aes.hpp>

#include <mutex>

namespace eos {

using chain::transaction_id_type;
using namespace boost::multi_index;
using types::Time;

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

      static const unsigned int max_message_size{128U << 10U};

      static const unsigned int max_rx_read{1U << 20U};
      char rxbuffer[max_rx_read + 16];  //read in to this buffer, add 16 bytes for AES block size carryover
      std::size_t rxbuffer_leftover{0};
      char parsebuffer[max_rx_read + max_message_size]; //decrypt to this buffer, with carry over for split messages
      size_t parsebuffer_leftover{0};

      //boost::asio::io_service::strand tx_strand;
      std::list<std::vector<uint8_t>> queuedOutgoing;

      std::mutex transaction_mutex;
      struct seen_transaction : public boost::noncopyable {
         transaction_id_type transaction_id;
         Time expires;
      };
      struct transaction_id{};
      struct expires{};
      boost::multi_index_container<
         seen_transaction,
         indexed_by<
           ordered_unique<tag<transaction_id>, BOOST_MULTI_INDEX_MEMBER(seen_transaction,transaction_id_type,transaction_id)>,
           ordered_non_unique<tag<expires>, BOOST_MULTI_INDEX_MEMBER(seen_transaction,Time,expires)>
         >
      > seen_transactions;

      signal<void()> on_disconnected_sig;
      bool disconnected_fired{false};    //be sure to only fire the signal once

      fc::aes_encoder    sending_aes_enc_ctx;
      fc::aes_decoder    receiving_aes_dec_ctx;
};

using tcp_connection_ptr = std::shared_ptr<tcp_connection>;

}