#pragma once

#include <eos/chain/protocol/types.hpp>
#include <eos/chain/protocol/authority.hpp>


/**
 *  @file sys_contract
 *  @brief defines logic for the native eos system contract
 */

namespace eos { namespace chain {


   struct Transfer {
      account_name to;
      uint64_t     amount = 0;
      string       memo;
   };

   struct CreateAccount {
      account_name  new_account;
      Authority     owner;
      Authority     active;
      uint64_t      initial_balance = 0;
   };


} }

FC_REFLECT( eos::chain::Transfer, (to)(amount)(memo) )
FC_REFLECT( eos::chain::CreateAccount, (new_account)(owner)(active)(initial_balance) )
