#include <eos/native_contract/producer_objects.hpp>
#include <eos/native_contract/staked_balance_objects.hpp>

#include <eos/chain/producer_object.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

namespace native {
namespace eos {
using namespace chain;
using namespace types;

void ProducerVotesObject::updateVotes(ShareType deltaVotes, UInt128 currentRaceTime) {
   auto timeSinceLastUpdate = currentRaceTime - race.positionUpdateTime;
   auto newPosition = race.position + race.speed * timeSinceLastUpdate;
   auto newSpeed = race.speed + deltaVotes;

   race.update(newSpeed, newPosition, currentRaceTime);
}

void ProxyVoteObject::addProxySource(const AccountName& source, ShareType sourceStake, chainbase::database& db) const {
   db.modify(*this, [&source, sourceStake](ProxyVoteObject& pvo) {
      pvo.proxySources.insert(source);
      pvo.proxiedStake += sourceStake;
   });
   db.get<StakedBalanceObject, byOwnerName>(proxyTarget).propagateVotes(sourceStake, db);
}

void ProxyVoteObject::removeProxySource(const AccountName& source, ShareType sourceStake,
                                        chainbase::database& db) const {
   db.modify(*this, [&source, sourceStake](ProxyVoteObject& pvo) {
      pvo.proxySources.erase(source);
      pvo.proxiedStake -= sourceStake;
   });
   db.get<StakedBalanceObject, byOwnerName>(proxyTarget).propagateVotes(sourceStake, db);
}

void ProxyVoteObject::updateProxiedStake(ShareType stakeDelta, chainbase::database& db) const {
   db.modify(*this, [stakeDelta](ProxyVoteObject& pvo) {
      pvo.proxiedStake += stakeDelta;
   });
   db.get<StakedBalanceObject, byOwnerName>(proxyTarget).propagateVotes(stakeDelta, db);
}

void ProxyVoteObject::cancelProxies(chainbase::database& db) const {
   boost::for_each(proxySources, [&db](const AccountName& source) {
      const auto& balance = db.get<StakedBalanceObject, byOwnerName>(source);
      db.modify(balance, [](StakedBalanceObject& sbo) {
         sbo.producerVotes = ProducerSlate{};
      });
   });
}

ProducerRound ProducerScheduleObject::calculateNextRound(chainbase::database& db) const {
   // Create storage and machinery with nice names, for choosing the top-voted producers
   ProducerRound round;
   auto FilterRetiredProducers = boost::adaptors::filtered([&db](const ProducerVotesObject& pvo) {
      return db.get<producer_object, by_owner>(pvo.ownerName).signing_key != PublicKey();
   });
   auto ProducerObjectToName = boost::adaptors::transformed([](const ProducerVotesObject& p) { return p.ownerName; });
   const auto& AllProducersByVotes = db.get_index<ProducerVotesMultiIndex, byVotes>();
   auto ActiveProducersByVotes = AllProducersByVotes | FilterRetiredProducers;

   FC_ASSERT(boost::distance(ActiveProducersByVotes) >= config::BlocksPerRound,
             "Not enough active producers registered to schedule a round!",
             ("ActiveProducers", (int64_t)boost::distance(ActiveProducersByVotes))
             ("AllProducers", (int64_t)AllProducersByVotes.size()));

   // Copy the top voted active producer's names into the round
   auto runnerUpStorage =
         boost::copy_n(ActiveProducersByVotes | ProducerObjectToName, config::VotedProducersPerRound, round.begin());

   // More machinery with nice names, this time for choosing runner-up producers
   auto VotedProducerRange = boost::make_iterator_range(round.begin(), runnerUpStorage);
   // Sort the voted producer names; we'll need to do it anyways, and it makes searching faster if we do it now
   boost::sort(VotedProducerRange);
   auto FilterVotedProducers = boost::adaptors::filtered([&VotedProducerRange](const ProducerVotesObject& pvo) {
      return !boost::binary_search(VotedProducerRange, pvo.ownerName);
   });
   const auto& AllProducersByFinishTime = db.get_index<ProducerVotesMultiIndex, byProjectedRaceFinishTime>();
   auto EligibleProducersByFinishTime = AllProducersByFinishTime | FilterRetiredProducers | FilterVotedProducers;

   auto runnerUpProducerCount = config::BlocksPerRound - config::VotedProducersPerRound;

   // Copy the front producers in the race into the round
   auto roundEnd =
         boost::copy_n(EligibleProducersByFinishTime | ProducerObjectToName, runnerUpProducerCount, runnerUpStorage);

   FC_ASSERT(roundEnd == round.end(),
             "Round scheduling yielded an unexpected number of producers: got ${actual}, but expected ${expected}",
             ("actual", (int64_t)std::distance(round.begin(), roundEnd))("expected", (int64_t)round.size()));
   auto lastRunnerUpName = *(roundEnd - 1);
   // Sort the runner-up producers into the voted ones
   boost::inplace_merge(round, runnerUpStorage);

   // Machinery to update the virtual race tracking for the producers that completed their lap
   auto lastRunnerUp = AllProducersByFinishTime.iterator_to(db.get<ProducerVotesObject,byOwnerName>(lastRunnerUpName));
   auto newRaceTime = lastRunnerUp->projectedRaceFinishTime();
   auto StartNewLap = [&db, newRaceTime](const ProducerVotesObject& pvo) {
      db.modify(pvo, [newRaceTime](ProducerVotesObject& pvo) {
         pvo.startNewRaceLap(newRaceTime);
      });
   };
   auto LapCompleters = boost::make_iterator_range(AllProducersByFinishTime.begin(), ++lastRunnerUp);

   // Start each producer that finished his lap on the next one, and update the global race time.
   try {
      if (boost::distance(LapCompleters) < AllProducersByFinishTime.size()
             && newRaceTime < std::numeric_limits<UInt128>::max()) {
         //ilog("Processed producer race. ${count} producers completed a lap at virtual time ${time}",
         //     ("count", (int64_t)boost::distance(LapCompleters))("time", newRaceTime));
         boost::for_each(LapCompleters, StartNewLap);
         db.modify(*this, [newRaceTime](ProducerScheduleObject& pso) {
            pso.currentRaceTime = newRaceTime;
         });
      } else {
         //wlog("Producer race finished; restarting race.");
         resetProducerRace(db);
      }
   } catch (ProducerRaceOverflowException&) {
      // Virtual race time has overflown. Reset race for everyone.
      // wlog("Producer race virtual time overflow detected! Resetting race.");
      resetProducerRace(db);
   }

   return round;
}

void ProducerScheduleObject::resetProducerRace(chainbase::database& db) const {
   auto ResetRace = [&db](const ProducerVotesObject& pvo) {
      db.modify(pvo, [](ProducerVotesObject& pvo) {
         pvo.startNewRaceLap(0);
      });
   };
   const auto& AllProducers = db.get_index<ProducerVotesMultiIndex, byVotes>();

   boost::for_each(AllProducers, ResetRace);
   db.modify(*this, [](ProducerScheduleObject& pso) {
      pso.currentRaceTime = 0;
   });
}

} } // namespace native::eos
