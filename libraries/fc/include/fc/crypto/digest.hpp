#pragma once
#include <fc/io/raw.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/crypto/sha256.hpp>

namespace fc {

   template<typename T>
   fc::sha256 digest( const T& value )
   {
      fc::sha256::encoder enc;
      fc::raw::pack( enc, value );
      return enc.result();
   }
}
