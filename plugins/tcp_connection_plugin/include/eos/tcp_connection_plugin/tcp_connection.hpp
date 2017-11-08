#pragma once

#include <eos/network_plugin/connection_interface.hpp>
#include <eosio/blockchain/types.hpp>
#include <eos/network_plugin/protocol.hpp>

#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/aes.hpp>

#include <mutex>

namespace eosio {

using blockchain::transaction_id_type;
using namespace boost::multi_index;
using namespace fc;

class tcp_connection : public connection_interface, public fc::visitor<void> {
   public:
      tcp_connection(boost::asio::ip::tcp::socket s);
      ~tcp_connection();
   
      bool disconnected() override;
      connection on_disconnected(const signal<void()>::slot_type& slot) override;

      void begin_processing_network_send_queue(connection_send_context& context) override;

      //set a callback to be fired when the connection is "open for business" -- any
      // initialization is done and it's ready to pass transactions, block, etc.
      void set_cb_on_ready(std::function<void()> cb);

      //RX message callbacks; public due to visitor
      void operator()(const handshake2_message& handshake);
      void operator()(const packed_transaction_list& transactions);

      //TX queue processors
      void operator()(const connection_send_failure& failure, connection_send_context& context);
      void operator()(const connection_send_nowork& nowork, connection_send_context& context);
      void operator()(const connection_send_transactions& transactions, connection_send_context& context);

   private:
      static const unsigned int max_message_size{16U << 20U};
      static const unsigned int max_rx_read{4U << 20U};
      static_assert(max_rx_read < max_message_size, "max_rx_read must be less than max_message_size");
      static_assert(!(max_message_size & (max_message_size-1)), "max_message_size must be power of two");
      static_assert(max_message_size < 1<<28, "max_message_size");
      
      struct seen_transaction : public boost::noncopyable {
         seen_transaction(transaction_id_type t, time_point_sec e) : transaction_id(t), expiration(e) {}
         transaction_id_type transaction_id;
         time_point_sec expiration;
      };
      struct by_trx_id{};
      struct by_expiration{};
      using transaction_multi_index = boost::multi_index_container<
         seen_transaction,
         indexed_by<
           hashed_unique<tag<by_trx_id>, BOOST_MULTI_INDEX_MEMBER(seen_transaction, transaction_id_type, transaction_id), std::hash<transaction_id_type>>,
           ordered_non_unique<tag<by_expiration>, BOOST_MULTI_INDEX_MEMBER(seen_transaction, time_point_sec, expiration)>
         >
      >;

      void finish_key_exchange(boost::system::error_code ec, size_t red, fc::ecc::private_key priv_key);

      void read();
      void read_ready(boost::system::error_code ec, size_t red);

      handshake2_message fill_handshake();

      void handle_failure();
      
      char _rxbuffer[max_rx_read + 16];  //read in to this buffer, add 16 bytes for AES block size carryover
      std::size_t _rxbuffer_leftover{0};

      char _parsebuffer[max_message_size*2]; //decrypt to this buffer
      size_t _parsebuffer_head{0};
      size_t _parsebuffer_tail{0};

      std::mutex _transaction_mutex;
      transaction_multi_index _seen_transactions;

      bool _currently_processing_network_queue{false};
      void get_next_network_work(connection_send_context& context);
      void prepare_and_send(net2_message message, connection_send_context& context);
      char _send_serialization_buffer[max_message_size+16];    //serialize in to here
      char _send_buffer[max_message_size];  //encrypt in to here (and send from here)
      void send_complete(const boost::system::error_code& ec, size_t wrote, connection_send_context& context);

      signal<void()> _on_disconnected_sig;
      bool _disconnected_fired{false};    //be sure to only fire the signal once

      fc::aes_encoder    _sending_aes_enc_ctx;
      fc::aes_decoder    _receiving_aes_dec_ctx;
      boost::asio::ip::tcp::socket _socket;
      boost::asio::io_service::strand _strand;
};

}