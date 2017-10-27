#include <eos/tcp_connection_plugin/tcp_connection.hpp>

#include <fc/log/logger.hpp>

#include <boost/asio.hpp>

namespace eos {

tcp_connection::tcp_connection(boost::asio::ip::tcp::socket s) :
   socket(std::move(s)),
   strand(socket.get_io_service())
{
   ilog("tcp_connection created");

   fc::ecc::private_key priv_key = fc::ecc::private_key::generate();
   
   fc::ecc::public_key pub = priv_key.get_public_key();
   fc::ecc::public_key_data pkd = pub.serialize();

   size_t wrote = boost::asio::write(socket, boost::asio::buffer(pkd.begin(), pkd.size()));
   if(wrote != pkd.size()) {
      handle_failure();
      return;
   }
   boost::asio::async_read(socket, boost::asio::buffer(rxbuffer, pkd.size()), [this, priv_key](auto ec, auto r) {
      finish_key_exchange(ec, r, priv_key);
   });
}

tcp_connection::~tcp_connection() {
   ilog("Connection destroyed"); //XXX debug
}

void tcp_connection::finish_key_exchange(boost::system::error_code ec, size_t red, fc::ecc::private_key priv_key) {
   fc::ecc::public_key_data* rpub = (fc::ecc::public_key_data*)rxbuffer;
   if(ec || red != rpub->size()) {
       handle_failure();
       return;
   }

   fc::sha512 shared_secret;
   try {
      shared_secret = priv_key.get_shared_secret(*rpub);
   }
   catch(fc::assert_exception) {
       elog("Failed to negotiate shared encrpytion secret with ${ip}:${port}. Not an EOSIO peer?",
            ("ip", socket.remote_endpoint().address().to_string())
            ("port", socket.remote_endpoint().port())
           );
       handle_failure();
       return;
   }

   auto ss_data = shared_secret.data();
   auto ss_data_size = shared_secret.data_size();

   sending_aes_enc_ctx.init  (fc::sha256::hash(ss_data, ss_data_size), fc::city_hash_crc_128(ss_data, ss_data_size));
   receiving_aes_dec_ctx.init(fc::sha256::hash(ss_data, ss_data_size), fc::city_hash_crc_128(ss_data, ss_data_size));

   read();
}

//Threading strategy: The RX and TX callbacks will not be wrapped -- they may indeed be called simulatnously
// from different threads. However, need to make sure that any calls _back_ to the socket object are
// serialized. This includes new async_reads, async_writes, or even close()s.

void tcp_connection::handle_failure() {
   strand.dispatch([this]() {
      socket.cancel();
      socket.close();
      if(!disconnected_fired)
         on_disconnected_sig();
      disconnected_fired = true;
   });
}

void tcp_connection::read() {
   strand.dispatch([this]() {
      auto read_buffer_offset = boost::asio::mutable_buffers_1(boost::asio::buffer(rxbuffer)+rxbuffer_leftover);
      socket.async_read_some(read_buffer_offset, [this](auto ec, auto r) {
         read_ready(ec, r);
      });
   });
}

void tcp_connection::read_ready(boost::system::error_code ec, size_t red) {
   if(ec) {
      handle_failure();
      return;
   }
   //the amount read (red), plus the amount left over from last time (rxbuffer_leftover), is what we
   // have to work with entirely
   unsigned int amount_in_rxbuffer = red+rxbuffer_leftover;
   //but we will only actually decode up to last multiple of AES block size (16 bytes)
   unsigned int amount_decryptable_in_rxbuffer = amount_in_rxbuffer & ~15U;

   receiving_aes_dec_ctx.decode(rxbuffer, amount_decryptable_in_rxbuffer, parsebuffer+parsebuffer_leftover);

   //whatever 0-15 bytes are leftover in rxbuffer; save them to next time through
   rxbuffer_leftover = amount_in_rxbuffer % 16;
   memmove(rxbuffer, rxbuffer+amount_decryptable_in_rxbuffer, rxbuffer_leftover);

   unsigned int successfully_parsed_bytes = 0;
   //the amount of data left in parsebuffer from last time around (parsebuffer_leftover) plus the
   // number of bytes we decrypted in this pass (amount_decryptable_in_rxbuffer) is what we should try to parse
   unsigned int parseable_bytes = parsebuffer_leftover+amount_decryptable_in_rxbuffer;
   datastream<const char*> parse_me(parsebuffer, parseable_bytes);
   do {
      net2_message message;
      try {
         fc::raw::unpack(parse_me, message);
         message.visit(*this);
      }
      catch(fc::out_of_range_exception& e) {
         //the entire message hasn't arrived yet; we're done processing this go around
         break;
      }
      catch(fc::exception& e) {
         //any other exception from unpacking is grounds for connection termination
         handle_failure();
         return;
      }
      //if we're here, we successfully got the entire message; dispatch it and move
      // up to the next 16 byte boundary
      if(parse_me.tellp()%16)
         parse_me.skip(16 - (parse_me.tellp()%16));
      successfully_parsed_bytes = parse_me.tellp();
   } while(parse_me.remaining());

   if(successfully_parsed_bytes == 0 && parseable_bytes > max_message_size) {
       //Something is wedged, we're not making progress.
       handle_failure();
       return;
   }

   //retain partially recieved message for next time around
   parsebuffer_leftover = parseable_bytes - successfully_parsed_bytes;
   memmove(parsebuffer, parsebuffer+successfully_parsed_bytes, parsebuffer_leftover);

   read();
}

void tcp_connection::operator()(const handshake2_message& handshake) {

}

void tcp_connection::operator()(const std::vector<DelimitingSignedTransaction>& transactions) {
   std::lock_guard<std::mutex> lock(transaction_mutex);

   for(const DelimitingSignedTransaction& trx : transactions) {
      auto p = seen_transactions.emplace(trx.id(), trx.expiration);
      if(!p.second) //couldn't emplace; we've seen trx in/egress recently already
         continue;

      std::vector<char> packed_trx = std::vector<char>(trx.start, trx.end);
   }
}

void tcp_connection::send_complete(boost::system::error_code ec, size_t sent, std::list<std::vector<uint8_t>>::iterator it) {
   if(ec) {
      handle_failure();
      return;
   }
   //sent should always == sent requested unless error, right? because _write() not _send_some()etc
   assert(sent == it->size());
   queuedOutgoing.erase(it);
}

bool tcp_connection::disconnected() {
   return !socket.is_open();
}

connection tcp_connection::on_disconnected(const signal<void()>::slot_type& slot) {
   return on_disconnected_sig.connect(slot);
}

handshake2_message tcp_connection::fill_handshake() {
   handshake2_message hand;

   hand.protocol_message_level = net2_message::count();
   app().get_plugin<chain_plugin>().get_chain_id(hand.chain_id);;

   return hand;
}

}