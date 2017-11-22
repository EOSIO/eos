/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/message_handling_contexts.hpp>

#include <eos/types/types.hpp>

namespace native {
namespace eosio { ///< eos native currency contract
namespace chain = ::eosio::chain;
namespace types = ::eosio::types;

void apply_eos_newaccount(chain::apply_context& context);
void apply_eos_transfer(chain::apply_context& context);
void apply_eos_lock(chain::apply_context& context);
void apply_eos_claim(chain::apply_context&);
void apply_eos_unlock(chain::apply_context&);
void apply_eos_okproducer(chain::apply_context&);
void apply_eos_setproducer(chain::apply_context&);
void apply_eos_setproxy(chain::apply_context&);
void apply_eos_setcode(chain::apply_context&);
void apply_eos_updateauth(chain::apply_context&);
void apply_eos_deleteauth(chain::apply_context&);
void apply_eos_linkauth(chain::apply_context&);
void apply_eos_unlinkauth(chain::apply_context&);

} // namespace eosio
} // namespace native
