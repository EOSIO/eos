#include <eos/native_contract/eos_contract.hpp>
#include <eos/native_contract/balance_object.hpp>

#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

namespace eos {
using namespace chain;

void CreateAccount_Notify_Eos::validate_preconditions(precondition_validate_context& context) {
   auto create = context.msg.as<types::CreateAccount>();
   const auto& creatorBalance = context.db.get<BalanceObject, byOwnerName>(create.creator);
   EOS_ASSERT(creatorBalance.balance >= create.deposit.amount, message_validate_exception,
              "Creator '${c}' has insufficient funds to make account creation deposit of ${a}",
              ("c", create.creator)("a", create.deposit));
}

void ClaimUnlockedEos_Notify_Eos::apply(apply_context& context) {
   auto claim = context.msg.as<types::ClaimUnlockedEos>();
   const auto& claimant = context.db.get<BalanceObject, byOwnerName>(claim.account);
   context.mutable_db.modify(claimant, [&claim](BalanceObject& a) {
      a.balance += claim.amount;
   });
}

void CreateAccount_Notify_Eos::apply(apply_context& context) {
   auto create = context.msg.as<types::CreateAccount>();
   context.mutable_db.create<BalanceObject>([&create](BalanceObject& b) {
      b.ownerName = create.name;
      b.balance = create.deposit.amount;
   });
   const auto& creatorBalance = context.mutable_db.get<BalanceObject, byOwnerName>(create.creator);
   context.mutable_db.modify(creatorBalance, [&create](BalanceObject& b) {
      b.balance -= create.deposit.amount;
   });
}

void Transfer::validate(message_validate_context& context) {
   auto transfer = context.msg.as<types::Transfer>();
   try {
      EOS_ASSERT(transfer.amount > Asset(0), message_validate_exception, "Must transfer a positive amount");
      EOS_ASSERT(context.msg.has_notify(transfer.to), message_validate_exception, "Must notify recipient of transfer");
   } FC_CAPTURE_AND_RETHROW((transfer))
}

void Transfer::validate_preconditions(precondition_validate_context& context) {
   const auto& db = context.db;
   auto transfer = context.msg.as<types::Transfer>();

   try {
      db.get<account_object,by_name>(transfer.to); ///< make sure this exists
      const auto& from = db.get<BalanceObject, byOwnerName>(transfer.from);
      EOS_ASSERT(from.balance >= transfer.amount.amount, message_precondition_exception, "Insufficient Funds",
                 ("from.balance",from.balance)("transfer.amount",transfer.amount));
   } FC_CAPTURE_AND_RETHROW((transfer))
}

void Transfer::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto transfer = context.msg.as<types::Transfer>();
   const auto& from = db.get<BalanceObject, byOwnerName>(transfer.from);
   const auto& to = db.get<BalanceObject, byOwnerName>(transfer.to);
#warning TODO: move balance from account_object to an EOS-constract-specific object
   db.modify(from, [&](BalanceObject& a) {
      a.balance -= transfer.amount.amount;
   });
   db.modify(to, [&](BalanceObject& a) {
      a.balance += transfer.amount.amount;
   });
}

void TransferToLocked::validate(message_validate_context& context) {
   auto lock = context.msg.as<types::TransferToLocked>();
   EOS_ASSERT(lock.amount > 0, message_validate_exception, "Locked amount must be positive");
   EOS_ASSERT(lock.to == lock.from || context.msg.has_notify(lock.to),
              message_validate_exception, "Recipient account must be notified");
   EOS_ASSERT(context.msg.has_notify(config::StakedBalanceContractName), message_validate_exception,
              "Staked Balance Contract (${name}) must be notified", ("name", config::StakedBalanceContractName));
}

void TransferToLocked::validate_preconditions(precondition_validate_context& context) {
   auto lock = context.msg.as<types::TransferToLocked>();
   ShareType balance;
   try {
      const auto& sender = context.db.get<BalanceObject, byOwnerName>(lock.from);
      context.db.get<BalanceObject, byOwnerName>(lock.to);
      balance = sender.balance;
   } EOS_RECODE_EXC(fc::exception, message_precondition_exception)
   EOS_ASSERT(balance >= lock.amount, message_precondition_exception,
              "Account ${a} lacks sufficient funds to lock ${amt} EOS", ("a", lock.from)("amt", lock.amount));
}

void TransferToLocked::apply(apply_context& context) {
   auto lock = context.msg.as<types::TransferToLocked>();
   const auto& locker = context.db.get<BalanceObject, byOwnerName>(lock.from);
   context.mutable_db.modify(locker, [&lock](BalanceObject& a) {
      a.balance -= lock.amount;
   });
}

} // namespace eos
