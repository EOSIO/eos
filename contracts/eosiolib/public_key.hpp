#pragma once 
#include <eosiolib/varint.hpp>
#include <eosiolib/serialize.hpp>

namespace eosio {

   /**
   *  @defgroup publickeytype Public Key Type
   *  @ingroup types
   *  @brief Specifies public key type
   *
   *  @{
   */
   
   /**
    * EOSIO Public Key
    * @brief EOSIO Public Key
    */
   struct public_key {
      /**
       * Type of the public key, could be either K1 or R1
       * @brief Type of the public key
       */
      unsigned_int        type;

      /**
       * Bytes of the public key
       * 
       * @brief Bytes of the public key
       */
      std::array<char,33> data;

      EOSLIB_SERIALIZE( public_key, (type)(data) )
   };
   
}

/// @} publickeytype
