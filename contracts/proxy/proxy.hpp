/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;

namespace proxy {

   TABLE set_owner {
      name     owner;
      uint32_t delay;

      EOSLIB_SERIALIZE( set_owner, (owner)(delay) )
   };

   TABLE config {
      name     key;
      name     owner{};
      uint32_t delay = 0;
      uint32_t next_id = 0;

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
