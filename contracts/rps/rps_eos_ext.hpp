/*
 * Overriding eosio pack and unpack function to support rps objects  
 */
#pragma once
#include <eoslib/datastream.hpp>
#include <eoslib/raw.hpp>
#include <eoslib/eos.hpp>
#include <rps.hpp>

namespace eosio {

  /**
   * eosio native transfer
   */
  struct transfer {
    account_name from;
    account_name to;
    uint64_t amount;
    string memo;
  };

  /*
   * eosio native transfer message unpacking function
   */
  template<>
  transfer current_message<transfer>() {
     uint32_t msgsize = message_size();
     char* buffer = (char *)eosio::malloc(msgsize);
     assert(read_message(buffer, msgsize) == msgsize, "error reading transfer");
     datastream<char *> ds(buffer, msgsize);
     transfer value;
     raw::unpack(ds, value.from);
     raw::unpack(ds, value.to);
     raw::unpack(ds, value.amount);
     raw::unpack(ds, value.memo);
     eosio::free(buffer);
     return value;
  }

  template<>
  rps::reveal current_message<rps::reveal>() {
     uint32_t msgsize = message_size();
     char* buffer = (char *)eosio::malloc(msgsize);
     assert(read_message(buffer, msgsize) == msgsize, "error reading reveal");
     datastream<char *> ds(buffer, msgsize);
     rps::reveal value;
     raw::unpack(ds, value.game_id);
     raw::unpack(ds, value.by);
     raw::unpack(ds, value.move_val);
     raw::unpack(ds, value.nonce);
     eosio::free(buffer);
     return value;
  }

  namespace raw {
    template<typename Stream> inline void pack( Stream& ds, const checksum& v ) {
      for (int i = 0; i < 4; i++) {
        pack(ds, v.hash[i]);
      }
    }

    template<typename Stream> inline void unpack( Stream& ds, checksum& v)  {
      for (int i = 0; i < 4; i++) {
        unpack(ds, v.hash[i]);
      }
    }
   
    template<typename Stream> inline void pack( Stream& ds, const rps::moves& moves_val) {
      pack(ds, moves_val.moves_val_len);
      for (int i = 0; i < moves_val.moves_val_len; i++) {
        pack(ds, moves_val.moves_val[i]);
      }
      pack(ds, moves_val.nonces_len);
      for (int i = 0; i < moves_val.nonces_len; i++) {
        pack(ds, moves_val.nonces[i]);
      }
      pack(ds, moves_val.hashed_moves_len);
      for (int i = 0; i < moves_val.hashed_moves_len; i++) {
        pack(ds, moves_val.hashed_moves[i]);
      }
      pack(ds, moves_val.submit_turn);
      pack(ds, moves_val.reveal_turn);     
    }
    
    template<typename Stream> inline void pack( Stream& ds, const rps::game& g) {
      pack(ds, g.id);
      pack(ds, g.foe);
      pack(ds, g.host);
      pack(ds, g.host_moves);
      pack(ds, g.foe_moves);
      pack(ds, g.round);
      pack(ds, g.round_winner_len);
      for (int i = 0; i < g.round_winner_len; i++) {
        pack(ds, g.round_winner[i]);
      }
      pack(ds, g.game_winner);
      pack(ds, g.host_stake);
      pack(ds, g.foe_stake);
      pack(ds, g.created_time);
      pack(ds, g.is_active);
    }

    template<typename Stream> inline void unpack( Stream& ds, rps::moves& moves_val) {
      unpack(ds, moves_val.moves_val_len);
      for (int i = 0; i < moves_val.moves_val_len; i++) {
        unpack(ds, moves_val.moves_val[i]);
      }
      unpack(ds, moves_val.nonces_len);
      for (int i = 0; i < moves_val.nonces_len; i++) {
        unpack(ds, moves_val.nonces[i]);
      }
      unpack(ds, moves_val.hashed_moves_len);
      for (int i = 0; i < moves_val.hashed_moves_len; i++) {
        unpack(ds, moves_val.hashed_moves[i]);
      }
      unpack(ds, moves_val.submit_turn);
      unpack(ds, moves_val.reveal_turn);  
    }


    template<typename Stream> inline void unpack( Stream& ds, rps::game& g) {
      unpack(ds, g.id);
      unpack(ds, g.foe);
      unpack(ds, g.host);
      unpack(ds, g.host_moves);
      unpack(ds, g.foe_moves);
      unpack(ds, g.round);
      unpack(ds, g.round_winner_len);
      for (int i = 0; i < g.round_winner_len; i++) {
        unpack(ds, g.round_winner[i]);
      }
      unpack(ds, g.game_winner);
      unpack(ds, g.host_stake);
      unpack(ds, g.foe_stake);
      unpack(ds, g.created_time);
      unpack(ds, g.is_active);
    }
  }
 
  bytes value_to_bytes(const rps::game& g) {
    uint32_t max_size = g.get_pack_size();
    char* buffer = (char *)eosio::malloc(max_size);
    datastream<char *> ds(buffer, max_size);     
    raw::pack(ds, g);
    bytes b;
    b.len = ds.tellp();
    b.data = (uint8_t*)buffer;
    return b;
 }

 template<typename T>
 T bytes_to_value(const bytes& b) { return *reinterpret_cast<T*>(b.data); }

 template<>
 rps::game bytes_to_value<rps::game>(const bytes& b) {
    datastream<char *> ds((char*)b.data, b.len);
    rps::game value;
    raw::unpack(ds, value);
    return value;
 }


}