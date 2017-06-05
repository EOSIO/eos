#pragma once

#include <eos/chain/message_handling_contexts.hpp>

#include <eos/types/types.hpp>

namespace eos {

struct Transfer {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

struct TransferToLocked {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

void ClaimUnlockedEos_Notify_Eos(chain::apply_context& context);

} // namespace eos
