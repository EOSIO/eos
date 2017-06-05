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
   db.modify(from, [&](account_object& a) {
      a.balance -= transfer.amount;
   });
   db.modify(to, [&](account_object& a) {
      a.balance += transfer.amount;
   });
}

} // namespace eos
