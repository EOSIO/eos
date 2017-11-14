/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/apply_context.hpp>

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain { namespace contracts { 

   void apply_eos_newaccount(apply_context& context);
   void apply_eos_transfer(apply_context& context);
   void apply_eos_lock(apply_context& context);
   void apply_eos_claim(apply_context&);
   void apply_eos_unlock(apply_context&);
   void apply_eos_okproducer(apply_context&);
   void apply_eos_setproducer(apply_context&);
   void apply_eos_setproxy(apply_context&);
   void apply_eos_setcode(apply_context&);
   void apply_eos_setabi(apply_context&);
   void apply_eos_updateauth(apply_context&);
   void apply_eos_deleteauth(apply_context&);
   void apply_eos_linkauth(apply_context&);
   void apply_eos_unlinkauth(apply_context&);
   void apply_eos_nonce(apply_context&);

} } } /// namespace eosio::contracts
