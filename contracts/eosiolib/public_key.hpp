#pragma once 
#include <eosiolib/varint.hpp>
#include <eosiolib/serialize.hpp>

namespace eosio {
   struct public_key {
      unsigned_int        type;
      std::array<char,33> data;

      EOSLIB_SERIALIZE( public_key, (type)(data) )
   };
}
