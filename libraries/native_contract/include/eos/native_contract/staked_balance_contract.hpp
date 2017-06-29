#pragma once

#include <eos/chain/message_handling_contexts.hpp>

#include <eos/types/types.hpp>

namespace native { namespace staked {
using namespace ::eos::chain;
namespace chain = ::eos::chain;
namespace types = ::eos::types;

/**
 *  Handle actions delivered to @system account when @staked account is notified
 */
///@{
//void precondition_system_newaccount(chain::precondition_validate_context&);
void apply_system_newaccount(chain::apply_context& context);
///@}

/**
 *  Handle actions delivered to @eos account when @staked account is notified
 */
///@{
//void precondition_eos_lock(chain::precondition_validate_context&);
void apply_eos_lock(chain::apply_context& context);
///@}


/**
 *  Handle actions delivered to @staked account when @staked account is notified
 */
///@{
void validate_staked_claim(chain::message_validate_context& context);
void precondition_staked_claim(chain::precondition_validate_context& context);
void apply_staked_claim(chain::apply_context& context);

void validate_staked_unlock(chain::message_validate_context& context);
void precondition_staked_unlock(chain::precondition_validate_context& context);
void apply_staked_unlock(chain::apply_context& context);

void validate_staked_setproducer(chain::message_validate_context& context);
void precondition_staked_setproducer(chain::precondition_validate_context& context);
void apply_staked_setproducer(chain::apply_context& context);


void validate_staked_okproducer(chain::message_validate_context& context);
void precondition_staked_okproducer(chain::precondition_validate_context& context);
void apply_staked_okproducer(chain::apply_context& context);

//void validate_staked_setproxy(chain::message_validate_context&) {}
void precondition_staked_setproxy(chain::precondition_validate_context& context);
void apply_staked_setproxy(chain::apply_context& context);
///@}


} } // namespace native::staked
