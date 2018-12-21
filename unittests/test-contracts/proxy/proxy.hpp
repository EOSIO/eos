/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace proxy {

   TABLE set_owner {
      name     owner;
      uint32_t delay;

      EOSLIB_SERIALIZE( set_owner, (owner)(delay) )
   };

   TABLE config {
      static constexpr name key = "config"_n;
      
      capi_name owner   = 0;
      uint32_t  delay   = 0;
      uint32_t  next_id = 0;

      uint64_t primary_key() const { return key.value; }

      EOSLIB_SERIALIZE( config, (key)(owner)(delay)(next_id) )
   };

   struct transfer_args {
      name        from;
      name        to;
      asset       quantity;
      std::string memo;
   };

} /// namespace proxy
