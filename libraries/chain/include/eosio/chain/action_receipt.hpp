/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

   /**
    *  For each action dispatched this receipt is generated
    */
   struct action_receipt {
      account_name                    receiver;
      digest_type                     act_digest;
      uint64_t                        global_sequence = 0; ///< total number of actions dispatched since genesis
      uint64_t                        recv_sequence = 0; ///< total number of actions with this receiver since genesis
      flat_map<account_name,uint64_t> auth_sequence;

      digest_type digest()const { return digest_type::hash(*this); }
   };

} }  /// namespace eosio::chain

FC_REFLECT( eosio::chain::action_receipt, (receiver)(act_digest)(global_sequence)(recv_sequence)(auth_sequence) )
