#pragma once 
#include <enulib/varint.hpp>
#include <enulib/serialize.hpp>

namespace enumivo {
   struct public_key {
      unsigned_int        type;
      std::array<char,33> data;

      friend bool operator == ( const public_key& a, const public_key& b ) {
        return std::tie(a.type,a.data) == std::tie(b.type,b.data);
      }
      friend bool operator != ( const public_key& a, const public_key& b ) {
        return std::tie(a.type,a.data) != std::tie(b.type,b.data);
      }
      ENULIB_SERIALIZE( public_key, (type)(data) )
   };
}
