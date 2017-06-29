#pragma once

#include <eos/chain/message_handling_contexts.hpp>

#include <eos/types/types.hpp>

namespace native {
namespace system {
using namespace ::eos::chain;
namespace chain = ::eos::chain;
namespace types = ::eos::types;

void validate_system_setcode(chain::message_validate_context& context);
void precondition_system_setcode(chain::precondition_validate_context& context);
void apply_system_setcode(chain::apply_context& context);

void validate_system_newaccount(chain::message_validate_context& context);
void precondition_system_newaccount(chain::precondition_validate_context& context);
void apply_system_newaccount(chain::apply_context& context);

} // namespace system
} // namespace native
