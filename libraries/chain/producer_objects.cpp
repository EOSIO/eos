/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/chain/producer_objects.hpp>
#include <eos/chain/staked_balance_objects.hpp>

#include <eos/chain/producer_object.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

namespace eosio {
namespace chain {
using namespace types;

void producer_votes_object::update_votes(share_type deltaVotes, uint128 current_race_time) {
   auto timeSinceLastUpdate = current_race_time - race.position_update_time;
   auto newPosition = race.position + race.speed * timeSinceLastUpdate;
   auto newSpeed = race.speed + deltaVotes;

   race.update(newSpeed, newPosition, current_race_time);
}

void proxy_vote_object::add_proxy_source(const account_name& source, share_type sourceStake, chainbase::database& db) const {
   db.modify(*this, [&source, sourceStake](proxy_vote_object& pvo) {
      pvo.proxy_sources.insert(source);
      pvo.proxied_stake += sourceStake;
   });
   db.get<staked_balance_object, by_owner_name>(proxy_target).propagate_votes(sourceStake, db);
}

void proxy_vote_object::removeProxySource(const account_name& source, share_type sourceStake,
                                        chainbase::database& db) const {
   db.modify(*this, [&source, sourceStake](proxy_vote_object& pvo) {
      pvo.proxy_sources.erase(source);
      pvo.proxied_stake -= sourceStake;
   });
   db.get<staked_balance_object, by_owner_name>(proxy_target).propagate_votes(sourceStake, db);
}

void proxy_vote_object::update_proxied_stake(share_type stakeDelta, chainbase::database& db) const {
   db.modify(*this, [stakeDelta](proxy_vote_object& pvo) {
      pvo.proxied_stake += stakeDelta;
   });
   db.get<staked_balance_object, by_owner_name>(proxy_target).propagate_votes(stakeDelta, db);
}

void proxy_vote_object::cancel_proxies(chainbase::database& db) const {
   boost::for_each(proxy_sources, [&db](const account_name& source) {
      const auto& balance = db.get<staked_balance_object, by_owner_name>(source);
      db.modify(balance, [](staked_balance_object& sbo) {
         sbo.producer_votes = producer_slate{};
      });
   });
}

producer_round producer_schedule_object::calculate_next_round(chainbase::database& db) const {
   // Create storage and machinery with nice names, for choosing the top-voted producers
   producer_round round;
   auto FilterRetiredProducers = boost::adaptors::filtered([&db](const producer_votes_object& pvo) {
      return db.get<producer_object, by_owner>(pvo.ownerName).signing_key != public_key();
   });
   auto ProducerObjectToName = boost::adaptors::transformed([](const producer_votes_object& p) { return p.ownerName; });
   const auto& AllProducersByVotes = db.get_index<producer_votes_multi_index, by_votes>();
   auto ActiveProducersByVotes = AllProducersByVotes | FilterRetiredProducers;

   FC_ASSERT(boost::distance(ActiveProducersByVotes) >= config::blocks_per_round,
             "Not enough active producers registered to schedule a round!",
             ("ActiveProducers", (int64_t)boost::distance(ActiveProducersByVotes))
             ("AllProducers", (int64_t)AllProducersByVotes.size()));

   // Copy the top voted active producer's names into the round
   auto runnerUpStorage =
         boost::copy_n(ActiveProducersByVotes | ProducerObjectToName, config::voted_producers_per_round, round.begin());

   // More machinery with nice names, this time for choosing runner-up producers
   auto VotedProducerRange = boost::make_iterator_range(round.begin(), runnerUpStorage);
   // Sort the voted producer names; we'll need to do it anyways, and it makes searching faster if we do it now
   boost::sort(VotedProducerRange);
   auto FilterVotedProducers = boost::adaptors::filtered([&VotedProducerRange](const producer_votes_object& pvo) {
      return !boost::binary_search(VotedProducerRange, pvo.ownerName);
   });
   const auto& AllProducersByFinishTime = db.get_index<producer_votes_multi_index, by_projected_race_finish_time>();
   auto EligibleProducersByFinishTime = AllProducersByFinishTime | FilterRetiredProducers | FilterVotedProducers;

   auto runnerUpProducerCount = config::blocks_per_round - config::voted_producers_per_round;

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
   auto lastRunnerUp = AllProducersByFinishTime.iterator_to(db.get<producer_votes_object,by_owner_name>(lastRunnerUpName));
   auto newRaceTime = lastRunnerUp->projected_race_finish_time();
   auto StartNewLap = [&db, newRaceTime](const producer_votes_object& pvo) {
      db.modify(pvo, [newRaceTime](producer_votes_object& pvo) {
         pvo.start_new_race_lap(newRaceTime);
      });
   };
   auto LapCompleters = boost::make_iterator_range(AllProducersByFinishTime.begin(), ++lastRunnerUp);

   // Start each producer that finished his lap on the next one, and update the global race time.
   try {
      if (boost::distance(LapCompleters) < AllProducersByFinishTime.size()
             && newRaceTime < std::numeric_limits<uint128>::max()) {
         //ilog("Processed producer race. ${count} producers completed a lap at virtual time ${time}",
         //     ("count", (int64_t)boost::distance(LapCompleters))("time", newRaceTime));
         boost::for_each(LapCompleters, StartNewLap);
         db.modify(*this, [newRaceTime](producer_schedule_object& pso) {
            pso.currentRaceTime = newRaceTime;
         });
      } else {
         //wlog("Producer race finished; restarting race.");
         reset_producer_race(db);
      }
   } catch (producer_race_overflow_exception&) {
      // Virtual race time has overflown. Reset race for everyone.
      // wlog("Producer race virtual time overflow detected! Resetting race.");
      reset_producer_race(db);
   }

   return round;
}

void producer_schedule_object::reset_producer_race(chainbase::database& db) const {
   auto ResetRace = [&db](const producer_votes_object& pvo) {
      db.modify(pvo, [](producer_votes_object& pvo) {
         pvo.start_new_race_lap(0);
      });
   };
   const auto& AllProducers = db.get_index<producer_votes_multi_index, by_votes>();

   boost::for_each(AllProducers, ResetRace);
   db.modify(*this, [](producer_schedule_object& pso) {
      pso.currentRaceTime = 0;
   });
}

} } // namespace eosio::chain
