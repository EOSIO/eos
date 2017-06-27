#include <eos/native_contract/staked_balance_contract.hpp>
#include <eos/native_contract/staked_balance_objects.hpp>
#include <eos/native_contract/producer_objects.hpp>

#include <eos/chain/global_property_object.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/binary_search.hpp>

namespace eos {
using namespace chain;

void CreateAccount_Notify_Staked::apply(apply_context& context) {
   auto create = context.msg.as<types::CreateAccount>();
   context.mutable_db.create<StakedBalanceObject>([&create](StakedBalanceObject& sbo) {
      sbo.ownerName = create.name;
   });
}

void TransferToLocked_Notify_Staked::apply(apply_context& context) {
   auto lock = context.msg.as<types::TransferToLocked>();
   const auto& balance = context.db.get<StakedBalanceObject, byOwnerName>(lock.to);

   balance.stakeTokens(lock.amount, context.mutable_db);
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
   const auto& balance = context.db.get<StakedBalanceObject, byOwnerName>(unlock.account);

   balance.beginUnstakingTokens(unlock.amount, context.mutable_db);
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
   auto approve = context.msg.as<types::ApproveProducer>();
   EOS_ASSERT(approve.approve == 0 || approve.approve == 1, message_validate_exception,
              "Unknown approval value: ${val}; must be either 0 or 1", ("val", approve.approve));
   EOS_ASSERT(approve.producer.size() != 0, message_validate_exception,
              "Approved producer's name cannot be empty");
}

void ApproveProducer::validate_preconditions(precondition_validate_context& context) {
   const auto& db = context.db;
   auto approve = context.msg.as<types::ApproveProducer>();
   auto producer = db.find<ProducerVotesObject, byOwnerName>(approve.producer);
   auto voter = db.find<StakedBalanceObject, byOwnerName>(context.msg.sender);

   EOS_ASSERT(producer != nullptr, message_precondition_exception,
              "Could not approve producer '${name}'; no such producer found", ("name", approve.producer));
   EOS_ASSERT(voter != nullptr, message_precondition_exception,
              "Could not find balance for '${name}'", ("name", context.msg.sender));
   EOS_ASSERT(voter->producerVotes.contains<ProducerSlate>(), message_precondition_exception,
              "Cannot approve producer; approving account '${name}' proxies its votes to '${proxy}'",
              ("name", voter->ownerName)("proxy", voter->producerVotes.get<AccountName>()));

   const auto& slate = voter->producerVotes.get<ProducerSlate>();

   EOS_ASSERT(slate.size < config::MaxProducerVotes, message_precondition_exception,
              "Cannot approve producer; approved producer count is already at maximum");
   if (approve.approve)
      EOS_ASSERT(!slate.contains(approve.producer), message_precondition_exception,
                 "Cannot add approval to producer '${name}'; producer is already approved",
                 ("name", approve.producer));
   else
      EOS_ASSERT(slate.contains(approve.producer), message_precondition_exception,
                 "Cannot remove approval from producer '${name}'; producer is not approved",
                 ("name", approve.producer));
}

void ApproveProducer::apply(apply_context& context) {
   auto& db = context.mutable_db;
   auto approve = context.msg.as<types::ApproveProducer>();
   const auto& producer = db.get<ProducerVotesObject, byOwnerName>(approve.producer);
   const auto& voter = db.get<StakedBalanceObject, byOwnerName>(context.msg.sender);
   auto raceTime = ProducerScheduleObject::get(db).currentRaceTime;
   auto totalVotingStake = voter.stakedBalance;

   // Check if voter is proxied to; if so, we need to add in the proxied stake
   if (auto proxy = db.find<ProxyVoteObject, byTargetName>(voter.ownerName))
      totalVotingStake += proxy->proxiedStake;

   // Add/remove votes from producer
   db.modify(producer, [approve = approve.approve, totalVotingStake, &raceTime](ProducerVotesObject& pvo) {
      if (approve)
         pvo.updateVotes(totalVotingStake, raceTime);
      else
         pvo.updateVotes(-totalVotingStake, raceTime);
   });
   // Add/remove producer from voter's approved producer list
   db.modify(voter, [&approve](StakedBalanceObject& sbo) {
      auto& slate = sbo.producerVotes.get<ProducerSlate>();
      if (approve.approve)
         slate.add(approve.producer);
      else
         slate.remove(approve.producer);
   });
}

void AllowVoteProxying::validate(message_validate_context& context) {
   auto allow = context.msg.as<types::AllowVoteProxying>();
   EOS_ASSERT(allow.allow == 0 || allow.allow == 1, message_validate_exception,
              "Unknown allow value: ${val}; must be either 0 or 1", ("val", allow.allow));
}

void AllowVoteProxying::validate_preconditions(precondition_validate_context& context) {
   auto allow = context.msg.as<types::AllowVoteProxying>();
   auto proxy = context.db.find<ProxyVoteObject, byTargetName>(context.msg.sender);
   if (allow.allow)
      EOS_ASSERT(proxy == nullptr, message_precondition_exception,
                 "Account '${name}' already allows vote proxying", ("name", context.msg.sender));
   else
      EOS_ASSERT(proxy != nullptr, message_precondition_exception,
                 "Account '${name}' already disallows vote proxying", ("name", context.msg.sender));
}

void AllowVoteProxying::apply(apply_context& context) {
   auto allow = context.msg.as<types::AllowVoteProxying>();
   auto& db = context.mutable_db;
   if (allow.allow)
      db.create<ProxyVoteObject>([target = context.msg.sender](ProxyVoteObject& pvo) {
         pvo.proxyTarget = target;
      });
   else {
      const auto& proxy = db.get<ProxyVoteObject, byTargetName>(context.msg.sender);
      proxy.cancelProxies(db);
      db.remove(proxy);
   }
}

void SetVoteProxy::validate_preconditions(precondition_validate_context& context) {
   auto svp = context.msg.as<types::SetVoteProxy>();
   const auto& db = context.db;

   auto proxy = db.find<ProxyVoteObject, byTargetName>(context.msg.sender);
   EOS_ASSERT(proxy == nullptr, message_precondition_exception,
              "Account '${src}' cannot proxy its votes, since it allows other accounts to proxy to it",
              ("src", context.msg.sender));

   if (svp.proxy != context.msg.sender) {
      // We are trying to enable proxying to svp.proxy
      auto proxy = db.find<ProxyVoteObject, byTargetName>(svp.proxy);
      EOS_ASSERT(proxy != nullptr, message_precondition_exception,
                 "Proxy target '${target}' does not allow votes to be proxied to it", ("target", svp.proxy));
   } else {
      // We are trying to disable proxying to sender.producerVotes.get<AccountName>()
      const auto& sender = db.get<StakedBalanceObject, byOwnerName>(context.msg.sender);
      EOS_ASSERT(sender.producerVotes.contains<AccountName>(), message_precondition_exception,
                 "Account '${name}' does not currently proxy its votes to any account", ("name", context.msg.sender));
   }
}

void SetVoteProxy::apply(apply_context& context) {
   auto svp = context.msg.as<types::SetVoteProxy>();
   auto& db = context.mutable_db;
   const auto& proxy = db.get<ProxyVoteObject, byTargetName>(svp.proxy);
   const auto& balance = db.get<StakedBalanceObject, byOwnerName>(context.msg.sender);

   if (svp.proxy != context.msg.sender) {
      // We are enabling proxying to svp.proxy
      proxy.addProxySource(context.msg.sender, balance.stakedBalance, db);
      db.modify(balance, [target = svp.proxy](StakedBalanceObject& sbo) { sbo.producerVotes = target; });
   } else {
      // We are disabling proxying to balance.producerVotes.get<AccountName>()
      proxy.removeProxySource(context.msg.sender, balance.stakedBalance, db);
      db.modify(balance, [](StakedBalanceObject& sbo) { sbo.producerVotes = ProducerSlate{}; });
   }
}

} // namespace eos
