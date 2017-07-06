#include <eos/native_contract/staked_balance_contract.hpp>
#include <eos/native_contract/staked_balance_objects.hpp>
#include <eos/native_contract/producer_objects.hpp>

#include <eos/chain/global_property_object.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/exceptions.hpp>

#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/binary_search.hpp>

namespace native {
namespace staked {


void apply_system_newaccount( apply_context& context ) {
   auto create = context.msg.as<types::newaccount>();
   context.mutable_db.create<StakedBalanceObject>([&create](StakedBalanceObject& sbo) {
      sbo.ownerName = create.name;
      sbo.stakedBalance = create.deposit.amount;
   });
}

void apply_eos_lock(apply_context& context) {
   auto lock = context.msg.as<types::lock>();
   const auto& balance = context.db.get<StakedBalanceObject, byOwnerName>(lock.to);

   balance.stakeTokens(lock.amount, context.mutable_db);
}

void validate_staked_unlock(message_validate_context& context) {
   auto unlock = context.msg.as<types::unlock>();
   EOS_ASSERT(unlock.amount >= 0, message_validate_exception, "Unlock amount cannot be negative");
}

void precondition_staked_unlock(precondition_validate_context& context) {
   auto unlock = context.msg.as<types::unlock>();
   ShareType balance;
   try {
      balance = context.db.get<StakedBalanceObject, byOwnerName>(unlock.account).stakedBalance;
   } EOS_RECODE_EXC(fc::exception, message_precondition_exception)
   EOS_ASSERT(balance >= unlock.amount, message_precondition_exception,
              "Insufficient locked funds to unlock ${a}", ("a", unlock.amount));
}

void apply_staked_unlock(apply_context& context) {
   auto unlock = context.msg.as<types::unlock>();
   const auto& balance = context.db.get<StakedBalanceObject, byOwnerName>(unlock.account);

   balance.beginUnstakingTokens(unlock.amount, context.mutable_db);
}

void validate_staked_claim(message_validate_context& context) {
   auto claim = context.msg.as<types::claim>();
   EOS_ASSERT(claim.amount > 0, message_validate_exception, "Claim amount must be positive");
   EOS_ASSERT(context.msg.has_notify(config::EosContractName), message_validate_exception,
              "EOS Contract (${name}) must be notified", ("name", config::EosContractName));
}

void precondition_staked_claim(precondition_validate_context& context) {
   auto claim = context.msg.as<types::claim>();
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

void apply_staked_claim(apply_context& context) {
   auto claim = context.msg.as<types::claim>();
   context.mutable_db.modify(context.db.get<StakedBalanceObject, byOwnerName>(claim.account),
                             [&claim](StakedBalanceObject& sbo) {
      sbo.unstakingBalance -= claim.amount;
   });
}

void validate_staked_setproducer(message_validate_context& context) {
   auto update = context.msg.as<types::setproducer>();
   EOS_ASSERT(update.name.good(), message_validate_exception, "Producer owner name cannot be empty");
}

void precondition_staked_setproducer(precondition_validate_context& context) {
   auto update = context.msg.as<types::setproducer>();
   const auto& db = context.db;
   auto producer = db.find<producer_object, by_owner>(update.name);
   if (producer)
      EOS_ASSERT(producer->signing_key != update.key || producer->configuration != update.configuration,
                 message_validate_exception, "Producer's new settings may not be identical to old settings");
}

void apply_staked_setproducer(apply_context& context) {
   auto update = context.msg.as<types::setproducer>();
   auto& db = context.mutable_db;
   auto producer = db.find<producer_object, by_owner>(update.name);

   if (producer)
      db.modify(*producer, [&update](producer_object& p) {
         p.signing_key = update.key;
         p.configuration = update.configuration;
      });
   else {
      db.create<producer_object>([&update](producer_object& p) {
         p.owner = update.name;
         p.signing_key = update.key;
         p.configuration = update.configuration;
      });
      auto raceTime = ProducerScheduleObject::get(db).currentRaceTime;
      db.create<ProducerVotesObject>([name = update.name, raceTime](ProducerVotesObject& pvo) {
         pvo.ownerName = name;
         pvo.startNewRaceLap(raceTime);
      });
   }
}

void validate_staked_okproducer(message_validate_context& context) {
   auto approve = context.msg.as<types::okproducer>();
   EOS_ASSERT(approve.approve == 0 || approve.approve == 1, message_validate_exception,
              "Unknown approval value: ${val}; must be either 0 or 1", ("val", approve.approve));
   context.msg.recipient(approve.voter);
   context.msg.recipient(approve.producer);
}

void precondition_staked_okproducer(precondition_validate_context& context) {
   const auto& db = context.db;
   auto approve = context.msg.as<types::okproducer>();
   auto producer = db.find<ProducerVotesObject, byOwnerName>(context.msg.recipient(approve.producer));
   auto voter = db.find<StakedBalanceObject, byOwnerName>(context.msg.recipient(approve.voter));

   EOS_ASSERT(producer != nullptr, message_precondition_exception,
              "Could not approve producer '${name}'; no such producer found", ("name", producer->ownerName));
   EOS_ASSERT(voter != nullptr, message_precondition_exception,
              "Could not find balance for '${name}'", ("name", voter->ownerName));
   EOS_ASSERT(voter->producerVotes.contains<ProducerSlate>(), message_precondition_exception,
              "Cannot approve producer; approving account '${name}' proxies its votes to '${proxy}'",
              ("name", voter->ownerName)("proxy", voter->producerVotes.get<AccountName>()));

   const auto& slate = voter->producerVotes.get<ProducerSlate>();

   EOS_ASSERT(slate.size < config::MaxProducerVotes, message_precondition_exception,
              "Cannot approve producer; approved producer count is already at maximum");
   if (approve.approve)
      EOS_ASSERT(!slate.contains(producer->ownerName), message_precondition_exception,
                 "Cannot add approval to producer '${name}'; producer is already approved",
                 ("name", producer->ownerName));
   else
      EOS_ASSERT(slate.contains(producer->ownerName), message_precondition_exception,
                 "Cannot remove approval from producer '${name}'; producer is not approved",
                 ("name", producer->ownerName));
}

void apply_staked_okproducer(apply_context& context) {
   auto& db = context.mutable_db;
   auto approve = context.msg.as<types::okproducer>();
   const auto& producer = db.get<ProducerVotesObject, byOwnerName>(context.msg.recipient(approve.producer));
   const auto& voter = db.get<StakedBalanceObject, byOwnerName>(context.msg.recipient(approve.voter));
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
   db.modify(voter, [&approve, producer = producer.ownerName](StakedBalanceObject& sbo) {
      auto& slate = sbo.producerVotes.get<ProducerSlate>();
      if (approve.approve)
         slate.add(producer);
      else
         slate.remove(producer);
   });
}

void validate_staked_setproxy(message_validate_context& context) {
   auto svp = context.msg.as<types::setproxy>();
   context.msg.recipient(svp.stakeholder);
   context.msg.recipient(svp.proxy);
}

void precondition_staked_setproxy(precondition_validate_context& context) {
   auto svp = context.msg.as<types::setproxy>();
   const auto& db = context.db;

   auto proxy = db.find<ProxyVoteObject, byTargetName>(context.msg.recipient(svp.proxy));
   EOS_ASSERT(proxy == nullptr, message_precondition_exception,
              "Account '${src}' cannot proxy its votes, since it allows other accounts to proxy to it",
              ("src", context.msg.recipient(svp.stakeholder)));

   if (svp.proxy != svp.stakeholder) {
      // We are trying to enable proxying from svp.stakeholder to svp.proxy
      auto proxy = db.find<ProxyVoteObject, byTargetName>(context.msg.recipient(svp.proxy));
      EOS_ASSERT(proxy != nullptr, message_precondition_exception,
                 "Proxy target '${target}' does not allow votes to be proxied to it", ("target", proxy->proxyTarget));
   } else {
      // We are trying to disable proxying to sender.producerVotes.get<AccountName>()
      const auto& sender = db.get<StakedBalanceObject, byOwnerName>(context.msg.recipient(svp.stakeholder));
      EOS_ASSERT(sender.producerVotes.contains<AccountName>(), message_precondition_exception,
                 "Account '${name}' does not currently proxy its votes to any account",
                 ("name", context.msg.recipient(svp.stakeholder)));
   }
}

void apply_staked_setproxy(apply_context& context) {
   auto svp = context.msg.as<types::setproxy>();
   auto& db = context.mutable_db;
   const auto& proxy = db.get<ProxyVoteObject, byTargetName>(context.msg.recipient(svp.proxy));
   const auto& balance = db.get<StakedBalanceObject, byOwnerName>(context.msg.recipient(svp.stakeholder));

   if (svp.proxy != svp.stakeholder) {
      // We are enabling proxying to svp.proxy
      proxy.addProxySource(context.msg.recipient(svp.stakeholder), balance.stakedBalance, db);
      db.modify(balance, [target = proxy.proxyTarget](StakedBalanceObject& sbo) { sbo.producerVotes = target; });
   } else {
      // We are disabling proxying to balance.producerVotes.get<AccountName>()
      proxy.removeProxySource(context.msg.recipient(svp.stakeholder), balance.stakedBalance, db);
      db.modify(balance, [](StakedBalanceObject& sbo) { sbo.producerVotes = ProducerSlate{}; });
   }
}

} } // namespace native::staked
