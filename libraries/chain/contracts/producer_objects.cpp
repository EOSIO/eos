/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/producer_objects.hpp>
#include <eosio/chain/contracts/staked_balance_objects.hpp>

#include <eosio/chain/producer_object.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

namespace eosio { namespace chain { namespace contracts {


void proxy_vote_object::add_proxy_source(const account_name& source, share_type source_stake, chainbase::database& db) const {
   db.modify(*this, [&source, source_stake](proxy_vote_object& pvo) {
      pvo.proxy_sources.insert(source);
      pvo.proxied_stake += source_stake;
   });
   db.get<staked_balance_object, by_owner_name>(proxy_target).propagate_votes(source_stake, db);
}

void proxy_vote_object::remove_proxy_source(const account_name& source, share_type source_stake,
                                        chainbase::database& db) const {
   db.modify(*this, [&source, source_stake](proxy_vote_object& pvo) {
      pvo.proxy_sources.erase(source);
      pvo.proxied_stake -= source_stake;
   });
   db.get<staked_balance_object, by_owner_name>(proxy_target).propagate_votes(source_stake, db);
}

void proxy_vote_object::update_proxied_state(share_type staked_delta, chainbase::database& db) const {
   db.modify(*this, [staked_delta](proxy_vote_object& pvo) {
      pvo.proxied_stake += staked_delta;
   });
   db.get<staked_balance_object, by_owner_name>(proxy_target).propagate_votes(staked_delta, db);
}

void proxy_vote_object::cancel_proxies(chainbase::database& db) const {
   boost::for_each(proxy_sources, [&db](const account_name& source) {
      const auto& balance = db.get<staked_balance_object, by_owner_name>(source);
      db.modify(balance, [](staked_balance_object& sbo) {
         sbo.producer_votes = producer_slate{};
      });
   });
}

/*
producer_round producer_schedule_object::calculate_next_round(chainbase::database& db) const {
   // Create storage and machinery with nice names, for choosing the top-voted producers
   producer_round round;
   auto filter_retired_producers = boost::adaptors::filtered([&db](const producer_votes_object& pvo) {
      return db.get<producer_object, by_owner>(pvo.owner_name).signing_key != public_key();
   });
   auto produer_object_to_name = boost::adaptors::transformed([](const producer_votes_object& p) { return p.owner_name; });
   const auto& all_producers_by_votes = db.get_index<producer_votes_multi_index, by_votes>();
   auto active_producers_by_votes = all_producers_by_votes | filter_retired_producers;

   FC_ASSERT(boost::distance(active_producers_by_votes) >= config::blocks_per_round,
             "Not enough active producers registered to schedule a round!",
             ("active_producers", (int64_t)boost::distance(active_producers_by_votes))
             ("all_producers", (int64_t)all_producers_by_votes.size()));

   // Copy the top voted active producer's names into the round
   auto runner_up_storage =
         boost::copy_n(active_producers_by_votes | produer_object_to_name, config::voted_producers_per_round, round.begin());

   // More machinery with nice names, this time for choosing runner-up producers
   auto voted_producer_range = boost::make_iterator_range(round.begin(), runner_up_storage);
   // Sort the voted producer names; we'll need to do it anyways, and it makes searching faster if we do it now
   boost::sort(voted_producer_range);
   auto FilterVotedProducers = boost::adaptors::filtered([&voted_producer_range](const producer_votes_object& pvo) {
      return !boost::binary_search(voted_producer_range, pvo.owner_name);
   });
   const auto& all_producers_by_finish_time = db.get_index<producer_votes_multi_index, by_projected_race_finish_time>();
   auto eligible_producers_by_finish_time = all_producers_by_finish_time | filter_retired_producers | FilterVotedProducers;

   auto runnerUpProducerCount = config::blocks_per_round - config::voted_producers_per_round;

   // Copy the front producers in the race into the round
   auto round_end =
         boost::copy_n(eligible_producers_by_finish_time | produer_object_to_name, runnerUpProducerCount, runner_up_storage);

   FC_ASSERT(round_end == round.end(),
             "Round scheduling yielded an unexpected number of producers: got ${actual}, but expected ${expected}",
             ("actual", (int64_t)std::distance(round.begin(), round_end))("expected", (int64_t)round.size()));
   auto lastRunnerUpName = *(round_end - 1);
   // Sort the runner-up producers into the voted ones
   boost::inplace_merge(round, runner_up_storage);

   // Machinery to update the virtual race tracking for the producers that completed their lap
   auto lastRunnerUp = all_producers_by_finish_time.iterator_to(db.get<producer_votes_object,by_owner_name>(lastRunnerUpName));
   auto new_race_time = lastRunnerUp->projected_race_finish_time();
   auto start_new_lap = [&db, new_race_time](const producer_votes_object& pvo) {
      db.modify(pvo, [new_race_time](producer_votes_object& pvo) {
         pvo.start_new_race_lap(new_race_time);
      });
   };
   auto lap_completers = boost::make_iterator_range(all_producers_by_finish_time.begin(), ++lastRunnerUp);

   // Start each producer that finished his lap on the next one, and update the global race time.
   try {
      if (boost::distance(lap_completers) < all_producers_by_finish_time.size()
             && new_race_time < std::numeric_limits<uint128_t>::max()) {
         //ilog("Processed producer race. ${count} producers completed a lap at virtual time ${time}",
         //     ("count", (int64_t)boost::distance(lap_completers))("time", new_race_time));
         boost::for_each(lap_completers, start_new_lap);
         db.modify(*this, [new_race_time](producer_schedule_object& pso) {
            pso.current_race_time = new_race_time;
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
*/

} } } // namespace eosio::chain::contracts
