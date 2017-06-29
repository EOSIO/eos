#pragma once

#include <eos/chain/message_handling_contexts.hpp>

#include <eos/types/types.hpp>

namespace native {
namespace eos { ///< eos native currency contract
namespace chain = ::eos::chain;
namespace types = ::eos::types;

/// handle apply the newaccount message to the system contract when we are notified
void precondition_system_newaccount(chain::precondition_validate_context& context);
void apply_system_newaccount(chain::apply_context& context);

/*
void precondition_staked_unlock(chain::precondition_validate_context&) {}
void apply_staked_unlock(chain::apply_context& context);
*/

void validate_eos_transfer(chain::message_validate_context& context);
void precondition_eos_transfer(chain::precondition_validate_context& context);
void apply_eos_transfer(chain::apply_context& context);

void validate_eos_lock(chain::message_validate_context& context);
void precondition_eos_lock(chain::precondition_validate_context& context);
void apply_eos_lock(chain::apply_context& context);

void apply_staked_claim(chain::apply_context&);

} // namespace eos
} // namespace native
