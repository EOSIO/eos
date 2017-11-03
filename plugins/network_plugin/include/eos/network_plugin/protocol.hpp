#pragma once
#include <eosio/blockchain/types.hpp>
#include <eosio/blockchain/transaction.hpp>
#include <chrono>

#include <fc/reflect/reflect.hpp>
#include <fc/io/datastream.hpp>

namespace eosio {

using namespace blockchain;
using namespace fc;

struct handshake2_message {
      fc::unsigned_int  protocol_message_level;
      chain_id_type     chain_id;
      fc::sha256        node_id;
      string            p2p_address;
      uint32_t          last_irreversible_block_num = 0;
      block_id_type     last_irreversible_block_id;
      uint32_t          head_num = 0;
      block_id_type     head_id;
      string            os;
      string            agent;
};

struct packed_transaction_list {
   std::vector<std::reference_wrapper<meta_transaction_ptr>> outgoing_transactions;
   std::vector<meta_transaction_ptr> incoming_transactions;
};

template<typename Stream>
inline Stream& operator>>(Stream& s, packed_transaction_list& d) {
   unsigned_int size;
   fc::raw::unpack(s, size);
   for(unsigned int i = 0 ; i < size; ++i) {
      std::vector<char> packed;
      fc::raw::unpack(s, packed);
      
      signed_transaction incoming_transaction;
      fc::datastream<const char*> trx_stream(packed.data(), packed.size());
      fc::raw::unpack(trx_stream, incoming_transaction);
      d.incoming_transactions.emplace_back(std::make_shared<meta_transaction>(incoming_transaction));
      d.incoming_transactions.back()->packed = std::move(packed);
   }
   return s;
}
template<typename Stream>
inline Stream& operator<<(Stream& s, const packed_transaction_list& d) {
   unsigned int size = d.outgoing_transactions.size();
   fc::raw::pack(s, size);
   for(unsigned int i = 0; i < size; ++i)
      fc::raw::pack(s, d.outgoing_transactions[size].get()->packed);
   return s;
}

using net2_message = static_variant<
   handshake2_message,
   packed_transaction_list
>;

}

FC_REFLECT( eosio::handshake2_message,
            (protocol_message_level)(chain_id)(node_id)
            (p2p_address)
            (last_irreversible_block_num)(last_irreversible_block_id)
            (head_num)(head_id)
            (os)(agent) )
