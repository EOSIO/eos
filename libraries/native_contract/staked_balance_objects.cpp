#include <eos/native_contract/staked_balance_objects.hpp>
#include <eos/native_contract/producer_objects.hpp>

#include <eos/chain/global_property_object.hpp>

#include <boost/range/algorithm/for_each.hpp>

namespace native {
namespace eos {
using namespace chain;
using namespace types;

void StakedBalanceObject::stakeTokens(ShareType newStake, chainbase::database& db) const {
   // Update the staked balance
   db.modify(*this, [&newStake](StakedBalanceObject& sbo) {
      sbo.stakedBalance += newStake;
   });

   propagateVotes(newStake, db);
}

void StakedBalanceObject::beginUnstakingTokens(ShareType amount, chainbase::database& db) const {
   // Remember there might be stake left over from a previous, uncompleted unstaking in unstakingBalance
   auto deltaStake = unstakingBalance - amount;

   // Update actual stake balance and invariants around it
   stakeTokens(deltaStake, db);
   // Update stats for unstaking process
   db.modify(*this, [&amount, &db](StakedBalanceObject& sbo) {
      sbo.unstakingBalance = amount;
      sbo.lastUnstakingTime = db.get(dynamic_global_property_object::id_type()).time;
   });
}

void StakedBalanceObject::finishUnstakingTokens(ShareType amount, chainbase::database& db) const {
   db.modify(*this, [&amount](StakedBalanceObject& sbo) {
      sbo.unstakingBalance -= amount;
   });
}

void StakedBalanceObject::propagateVotes(ShareType stakeDelta, chainbase::database& db) const {
   if (producerVotes.contains<ProducerSlate>())
      // This account votes for producers directly; update their stakes
      boost::for_each(producerVotes.get<ProducerSlate>().range(), [&db, &stakeDelta](const AccountName& name) {
         db.modify(db.get<ProducerVotesObject, byOwnerName>(name), [&db, &stakeDelta](ProducerVotesObject& pvo) {
            pvo.updateVotes(stakeDelta, ProducerScheduleObject::get(db).currentRaceTime);
         });
      });
   else {
      // This account has proxied its votes to another account; update the ProxyVoteObject
      const auto& proxy = db.get<ProxyVoteObject, byTargetName>(producerVotes.get<AccountName>());
      proxy.updateProxiedStake(stakeDelta, db);
   }
}

} } // namespace native::eos
