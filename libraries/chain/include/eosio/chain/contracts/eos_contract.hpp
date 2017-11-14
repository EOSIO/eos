/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/apply_context.hpp>

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace contracts { 

   void apply_eosio_newaccount(apply_context& context);
   void apply_eosio_transfer(apply_context& context);
   void apply_eosio_lock(apply_context& context);
   void apply_eosio_claim(apply_context&);
   void apply_eosio_unlock(apply_context&);
   void apply_eosio_okproducer(apply_context&);
   void apply_eosio_setproducer(apply_context&);
   void apply_eosio_setproxy(apply_context&);
   void apply_eosio_setcode(apply_context&);
   void apply_eosio_setabi(apply_context&);
   void apply_eosio_updateauth(apply_context&);
   void apply_eosio_deleteauth(apply_context&);
   void apply_eosio_linkauth(apply_context&);
   void apply_eosio_unlinkauth(apply_context&);
   void apply_eosio_nonce(apply_context&);

} } } /// namespace eosio::contracts
