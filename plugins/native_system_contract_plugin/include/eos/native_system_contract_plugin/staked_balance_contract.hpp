#pragma once

#include <eos/chain/message_handling_contexts.hpp>

#include <eos/types/types.hpp>

namespace eos {

struct TransferToLocked {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

struct StartUnlockEos {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

struct ClaimUnlockedEos {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

struct CreateProducer {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

struct UpdateProducer {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

struct ApproveProducer {
   static void validate(chain::message_validate_context& context);
   static void validate_preconditions(chain::precondition_validate_context& context);
   static void apply(chain::apply_context& context);
};

} // namespace eos
