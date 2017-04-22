#pragma once

#include <eos/chain/protocol/types.hpp>


/**
 *  @file sys_contract
 *  @brief defines logic for the native eos system contract
 */

namespace eos { namespace chain {


   struct Transfer {
      account_name from;
      account_name to;
      uint64_t     amount = 0;
      string       memo;
   };


} }

FC_REFLECT( eos::chain::Transfer, (from)(to)(amount)(memo) )
