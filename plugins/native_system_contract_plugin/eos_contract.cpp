#include <eos/native_system_contract_plugin/eos_contract.hpp>

#include <eos/chain/message_handling_contexts.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

namespace eos {
using namespace chain;

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

   db.get<account_object,by_name>(transfer.to); ///< make sure this exists
   const auto& from = db.get<account_object,by_name>(transfer.from);
   EOS_ASSERT(from.balance >= transfer.amount.amount, message_precondition_exception, "Insufficient Funds",
              ("from.balance",from.balance)("transfer.amount",transfer.amount));
}

void Transfer::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto transfer = context.msg.as<types::Transfer>();
   const auto& from = db.get<account_object,by_name>(transfer.from);
   const auto& to = db.get<account_object,by_name>(transfer.to);
#warning TODO: move balance from account_object to an EOS-constract-specific object
   db.modify(from, [&](account_object& a) {
      a.balance -= transfer.amount;
   });
   db.modify(to, [&](account_object& a) {
      a.balance += transfer.amount;
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
      const auto& sender = context.db.get<account_object, by_name>(lock.from);
      context.db.get<account_object, by_name>(lock.to);
      balance = sender.balance.amount;
   } EOS_RECODE_EXC(fc::exception, message_precondition_exception)
   EOS_ASSERT(balance >= lock.amount, message_precondition_exception,
              "Account ${a} lacks sufficient funds to lock ${amt} EOS", ("a", lock.from)("amt", lock.amount));
}

void TransferToLocked::apply(apply_context& context) {
   auto lock = context.msg.as<types::TransferToLocked>();
   const auto& locker = context.db.get<account_object, by_name>(lock.from);
   context.mutable_db.modify(locker, [&lock](account_object& a) {
      a.balance.amount -= lock.amount;
   });
}

void ClaimUnlockedEos_Notify_Eos(apply_context& context) {
   auto claim = context.msg.as<types::ClaimUnlockedEos>();
   const auto& claimant = context.db.get<account_object, by_name>(claim.account);
   context.mutable_db.modify(claimant, [&claim](account_object& a) {
      a.balance.amount += claim.amount;
   });
}

} // namespace eos
