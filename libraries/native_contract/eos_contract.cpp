#include <eos/native_contract/eos_contract.hpp>
#include <eos/native_contract/balance_object.hpp>

#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/message.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

namespace native {
namespace eos {
using namespace ::eos::chain;
namespace config = ::eos::config;
namespace chain = ::eos::chain;

/**
 *  This method is called when the newaccount message is delivered to @system and @eos is notified.
 *
 *  validate_system_newaccount requires that @eos be notified so we can safely rely on this method
 *  always being called when a new account is created.
 *
 *  The purpose of this call is to verify the new account has the sufficient funds to populate the
 *  @staked balance required to reserve the space for the new account.
 */
void precondition_system_newaccount(chain::precondition_validate_context& context ) {
   auto create = context.msg.as<types::newaccount>();
   const auto& creatorBalance = context.db.get<BalanceObject, byOwnerName>(create.creator);
   EOS_ASSERT(creatorBalance.balance >= create.deposit.amount, message_validate_exception,
              "Creator '${c}' has insufficient funds to make account creation deposit of ${a}",
              ("c", create.creator)("a", create.deposit));
}

/**
 *  This method is called assuming precondition_system_newaccount succeeds and proceeds to
 *  deduct the balance of the account creator by deposit, this deposit is supposed to be
 *  credited to the staked balance the new account in the @staked contract.
 */
void apply_system_newaccount(apply_context& context) {
   auto create = context.msg.as<types::newaccount>();

   const auto& creatorBalance = context.mutable_db.get<BalanceObject, byOwnerName>(create.creator);
   context.mutable_db.modify(creatorBalance, [&create](BalanceObject& b) {
      b.balance -= create.deposit.amount;
   });

   context.mutable_db.create<BalanceObject>([&create](BalanceObject& b) {
      b.ownerName = create.name;
      b.balance = 0; //create.deposit.amount; TODO: make sure we credit this in @staked
   });
}

/**
 *  This method is called when the claim message is delivered to @staked 
 *  staked::validate_staked_claim must require that @eos be notified.
 *
 *  This method trusts that staked::precondition_staked_claim verifies that claim.amount is
 *  available.
 */
void apply_staked_claim(apply_context& context) {
   auto claim = context.msg.as<types::claim>();
   const auto& claimant = context.db.get<BalanceObject, byOwnerName>(claim.account);
   context.mutable_db.modify(claimant, [&claim](BalanceObject& a) {
      a.balance += claim.amount;
   });
}

/**
 *
 * @ingroup native_eos
 * @defgroup eos_eos_transfer eos::eos_transfer
 */
///@{
/**
 * When transferring EOS the amount must be positive and the receiver must be notified.
 */
void validate_eos_transfer(message_validate_context& context) {
#warning TODO: should transfer validate that the sender is in the provided authorites
   auto transfer = context.msg.as<types::transfer>();
   try {
      EOS_ASSERT(transfer.amount > Asset(0), message_validate_exception, "Must transfer a positive amount");
      EOS_ASSERT(context.msg.has_notify(transfer.to), message_validate_exception, "Must notify recipient of transfer");
   } FC_CAPTURE_AND_RETHROW((transfer))
}


void precondition_eos_transfer(precondition_validate_context& context) {
   const auto& db = context.db;
   auto transfer = context.msg.as<types::transfer>();

   try {
      db.get<account_object,by_name>(transfer.to); ///< make sure this exists
      const auto& from = db.get<BalanceObject, byOwnerName>(transfer.from);
      EOS_ASSERT(from.balance >= transfer.amount.amount, message_precondition_exception, "Insufficient Funds",
                 ("from.balance",from.balance)("transfer.amount",transfer.amount));
   } FC_CAPTURE_AND_RETHROW((transfer))
}

void apply_eos_transfer(apply_context& context) {
   auto& db = context.mutable_db;
   auto transfer = context.msg.as<types::transfer>();
   const auto& from = db.get<BalanceObject, byOwnerName>(transfer.from);
   const auto& to = db.get<BalanceObject, byOwnerName>(transfer.to);
   db.modify(from, [&](BalanceObject& a) {
      a.balance -= transfer.amount.amount;
   });
   db.modify(to, [&](BalanceObject& a) {
      a.balance += transfer.amount.amount;
   });
}
///@}


/**
 *  When the lock action is delivered to @eos the account should debit the locked amount
 *  and then require that @staked be notified. @staked will credit the amount to the
 *  locked balance and adjust votes accordingly.  
 *
 *  The locked amount must be possitive.
 *
 *  @defgroup eos_eos_lock eos::eos_lock
 */
///@{
void validate_eos_lock(message_validate_context& context) {
   auto lock = context.msg.as<types::lock>();
   EOS_ASSERT(lock.amount > 0, message_validate_exception, "Locked amount must be positive");
   EOS_ASSERT(lock.to == lock.from || context.msg.has_notify(lock.to),
              message_validate_exception, "Recipient account must be notified");
   EOS_ASSERT(context.msg.has_notify(config::StakedBalanceContractName), message_validate_exception,
              "Staked Balance Contract (${name}) must be notified", ("name", config::StakedBalanceContractName));
}

/**
 *  The the from account must have sufficient balance
 */
void precondition_eos_lock(precondition_validate_context& context) {
   auto lock = context.msg.as<types::lock>();
   ShareType balance;
   try {
      const auto& sender = context.db.get<BalanceObject, byOwnerName>(lock.from);
      context.db.get<BalanceObject, byOwnerName>(lock.to);
      balance = sender.balance;
   } EOS_RECODE_EXC(fc::exception, message_precondition_exception)
   EOS_ASSERT(balance >= lock.amount, message_precondition_exception,
              "Account ${a} lacks sufficient funds to lock ${amt} EOS", ("a", lock.from)("amt", lock.amount));
}

/**
 *  Deduct the balance from the from account.
 */
void apply_eos_lock(apply_context& context) {
   auto lock = context.msg.as<types::lock>();
   const auto& locker = context.db.get<BalanceObject, byOwnerName>(lock.from);
   context.mutable_db.modify(locker, [&lock](BalanceObject& a) {
      a.balance -= lock.amount;
   });
}
///@}

} // namespace eos
} // namespace native
