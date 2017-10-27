#pragma once
#include <eos/chain/block.hpp>
#include <eos/chain/types.hpp>
#include <chrono>

#include <fc/reflect/reflect.hpp>

namespace eos {
   
using namespace chain;
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

//XXX Abuse?
struct DelimitingSignedTransaction : public SignedTransaction {
   const char* start;
   const char* end;
};

template<typename Stream>
inline Stream& operator>>(Stream& s, DelimitingSignedTransaction& d) {
   d.start = s.pos();
   fc::raw::unpack<fc::datastream<const char *>, eos::chain::SignedTransaction>(s, d);
   d.end = s.pos();
   return s;
}
template<typename Stream>
inline Stream& operator<<(Stream& s, const DelimitingSignedTransaction& d) {
   fc::raw::pack<fc::datastream<char *>, eos::chain::SignedTransaction>(s, d);
   return s;
}

using net2_message = static_variant<
   handshake2_message,
   std::vector<DelimitingSignedTransaction>
>;

}

FC_REFLECT( eos::handshake2_message,
            (protocol_message_level)(chain_id)(node_id)
            (p2p_address)
            (last_irreversible_block_num)(last_irreversible_block_id)
            (head_num)(head_id)
            (os)(agent) )
