#include <eos/native_contract/staked_balance_contract.hpp>
#include <eos/native_contract/staked_balance_objects.hpp>
#include <eos/native_contract/producer_objects.hpp>

#include <eos/chain/global_property_object.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

namespace eos {
using namespace chain;

void CreateAccount_Notify_Staked::validate_preconditions(precondition_validate_context& context) {

}

void CreateAccount_Notify_Staked::apply(apply_context& context) {
   auto create = context.msg.as<types::CreateAccount>();
   context.mutable_db.create<StakedBalanceObject>([&create](StakedBalanceObject& sbo) {
      sbo.ownerName = create.name;
   });
}

void TransferToLocked_Notify_Staked::apply(apply_context& context) {
   auto lock = context.msg.as<types::TransferToLocked>();
   const auto& balance = context.db.get<StakedBalanceObject, byOwnerName>(lock.to);
   context.mutable_db.modify(balance, [&lock](StakedBalanceObject& sbo) {
      sbo.stakedBalance += lock.amount;
   });
#warning TODO: Update producer votes
}

void StartUnlockEos::validate(message_validate_context& context) {
   auto unlock = context.msg.as<types::StartUnlockEos>();
   EOS_ASSERT(unlock.amount >= 0, message_validate_exception, "Unlock amount cannot be negative");
}

void StartUnlockEos::validate_preconditions(precondition_validate_context& context) {
   auto unlock = context.msg.as<types::StartUnlockEos>();
   ShareType balance;
   try {
      balance = context.db.get<StakedBalanceObject, byOwnerName>(unlock.account).stakedBalance;
   } EOS_RECODE_EXC(fc::exception, message_precondition_exception)
   EOS_ASSERT(balance >= unlock.amount, message_precondition_exception,
              "Insufficient locked funds to unlock ${a}", ("a", unlock.amount));
}

void StartUnlockEos::apply(apply_context& context) {
   auto unlock = context.msg.as<types::StartUnlockEos>();
   context.mutable_db.modify(context.db.get<StakedBalanceObject, byOwnerName>(unlock.account),
                             [&unlock, &db = context.db](StakedBalanceObject& sbo) {
      // If there was any asset left unclaimed from a previous unstaking, move it back to staked first
      sbo.stakedBalance += sbo.unstakingBalance;
      // OK, now unstakingBalance is logically zero, so we can just overwrite it with its new value
      sbo.unstakingBalance = unlock.amount;
      // Deduct the now unstaking balance from the staked balance, and record the time
      sbo.stakedBalance -= unlock.amount;
      sbo.lastUnstakingTime = db.get(dynamic_global_property_object::id_type()).time;
   });
}

void ClaimUnlockedEos::validate(message_validate_context& context) {
   auto claim = context.msg.as<types::ClaimUnlockedEos>();
   EOS_ASSERT(claim.amount > 0, message_validate_exception, "Claim amount must be positive");
   EOS_ASSERT(context.msg.has_notify(config::EosContractName), message_validate_exception,
              "EOS Contract (${name}) must be notified", ("name", config::EosContractName));
}

void ClaimUnlockedEos::validate_preconditions(precondition_validate_context& context) {
   auto claim = context.msg.as<types::ClaimUnlockedEos>();
   auto balance = context.db.find<StakedBalanceObject, byOwnerName>(claim.account);
   EOS_ASSERT(balance != nullptr, message_precondition_exception,
              "Could not find staked balance for ${name}", ("name", claim.account));
   auto balanceReleaseTime = balance->lastUnstakingTime + config::StakedBalanceCooldownSeconds;
   auto now = context.db.get(dynamic_global_property_object::id_type()).time;
   EOS_ASSERT(now >= balanceReleaseTime, message_precondition_exception,
              "Cannot claim balance until ${releaseDate}", ("releaseDate", balanceReleaseTime));
   EOS_ASSERT(balance->unstakingBalance >= claim.amount, message_precondition_exception,
              "Cannot claim ${claimAmount} as only ${available} is available for claim",
              ("claimAmount", claim.amount)("available", balance->unstakingBalance));
}

void ClaimUnlockedEos::apply(apply_context& context) {
   auto claim = context.msg.as<types::ClaimUnlockedEos>();
   context.mutable_db.modify(context.db.get<StakedBalanceObject, byOwnerName>(claim.account),
                             [&claim](StakedBalanceObject& sbo) {
      sbo.unstakingBalance -= claim.amount;
   });
#warning TODO: Update producer votes
}

void CreateProducer::validate(message_validate_context& context) {
   auto create = context.msg.as<types::CreateProducer>();
   EOS_ASSERT(create.name.size() > 0, message_validate_exception, "Producer owner name cannot be empty");
}

void CreateProducer::validate_preconditions(precondition_validate_context& context) {
   auto create = context.msg.as<types::CreateProducer>();
   const auto& db = context.db;
   auto producer = db.find<producer_object, by_owner>(create.name);
   EOS_ASSERT(producer == nullptr, message_precondition_exception,
              "Account ${name} already has a block producer", ("name", create.name));
}

void CreateProducer::apply(apply_context& context) {
   auto create = context.msg.as<types::CreateProducer>();
   auto& db = context.mutable_db;
   db.create<producer_object>([&create](producer_object& p) {
      p.owner = create.name;
      p.signing_key = create.key;
      p.configuration = create.configuration;
   });
   auto raceTime = ProducerScheduleObject::get(db).currentRaceTime;
   db.create<ProducerVotesObject>([&create, &raceTime](ProducerVotesObject& pvo) {
      pvo.ownerName = create.name;
      pvo.startNewRaceLap(raceTime);
   });
}

void UpdateProducer::validate(message_validate_context& context) {
   auto update = context.msg.as<types::UpdateProducer>();
   EOS_ASSERT(update.name.size() > 0, message_validate_exception, "Producer owner name cannot be empty");
}

void UpdateProducer::validate_preconditions(precondition_validate_context& context) {
   const auto& db = context.db;
   auto update = context.msg.as<types::UpdateProducer>();
   const auto& producer = db.get<producer_object, by_owner>(update.name);
   EOS_ASSERT(producer.signing_key != update.newKey || producer.configuration != update.configuration,
              message_validate_exception, "Producer's new settings may not be identical to old settings");
}

void UpdateProducer::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto update = context.msg.as<types::UpdateProducer>();
   const auto& producer = db.get<producer_object, by_owner>(update.name);

   db.modify(producer, [&update](producer_object& p) {
      p.signing_key = update.newKey;
      p.configuration = update.configuration;
   });
}

void ApproveProducer::validate(message_validate_context& context) {
}

void ApproveProducer::validate_preconditions(precondition_validate_context& context) {
}

void ApproveProducer::apply(apply_context& context) {
}

} // namespace eos
