/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/types.hpp>
#include <eos/chain/multi_index_includes.hpp>

#include <eos/types/types.hpp>

#include <eos/utilities/exception_macros.hpp>

#include <chainbase/chainbase.hpp>

#include <boost/multi_index/mem_fun.hpp>

namespace eosio {
namespace chain {


FC_DECLARE_EXCEPTION(producer_race_overflow_exception, 10000000, "Producer Virtual Race time has overflowed");

/**
 * @brief The producer_votes_object class tracks all votes for and by the block producers
 *
 * This class tracks the voting for block producers, as well as the virtual time 'race' to select the runner-up block
 * producer.
 *
 * This class also tracks the votes cast by block producers on various chain configuration options and key documents.
 */
class producer_votes_object : public chainbase::object<producer_votes_object_type, producer_votes_object> {
   OBJECT_CTOR(producer_votes_object)

   id_type id;
   types::account_name ownerName;

   /**
    * @brief Update the tally of votes for the producer, while maintaining virtual time accounting
    *
    * This function will update the producer's position in the race
    *
    * @param deltaVotes The change in votes since the last update
    * @param current_race_time The current "race time"
    */
   void update_votes(types::share_type deltaVotes, types::uint128 current_race_time);
   /// @brief Get the number of votes this producer has received
   types::share_type get_votes() const { return race.speed; }
   pair<types::share_type,id_type> get_vote_order()const { return { race.speed, id }; }

   /**
    * These fields are used for the producer scheduling algorithm which uses a virtual race to ensure that runner-up
    * producers are given proportional time for producing blocks. Producers are constantly running a racetrack of
    * length config::producer_race_lap_length, at a speed equal to the number of votes they have received. Runner-up
    * producers, who lack sufficient votes to get in as a top-N voted producer, get scheduled to produce a block every
    * time they finish the race. The race algorithm ensures that runner-up producers are scheduled with a frequency
    * proportional to their relative vote tallies; i.e. a runner-up with more votes is scheduled more often than one
    * with fewer votes, but all runners-up with any votes will theoretically be scheduled eventually.
    *
    * Whenever a runner-up producer needs to be scheduled, the one projected to finish the race next is taken, with the
    * caveat that any producers already scheduled for other reasons (such as being a top-N voted producer) cannot be
    * scheduled a second time in the same round, and thus they are skipped/ignored when looking for the producer
    * projected to finish next. After selecting the runner-up for scheduling, the global current race time is updated
    * to the point at which the winner crossed the finish line (i.e. his position is zero: he is now beginning his next
    * lap). Furthermore, all producers which crossed the finish line ahead of the winner, but were ineligible to win as
    * they were already scheduled in the next round, are effectively held at the finish line until the winner crosses,
    * so their positions are all also set to zero.
    *
    * Every time the number of votes for a given producer changes, the producer's speed in the race changes and thus
    * his projected finishing time changes. In this event, the stats must be updated: first, the producer's position is
    * updated to reflect his progress since the last stats update to the current race time; second, the producer's
    * projected finishing time based on his new position and speed are updated so we know where he shakes out in the
    * new projected finish times.
    *
    * @warning Do not update these values directly; use @ref update_votes instead!
    */
   struct {
      /// The current speed for this producer (which is actually the total votes for the producer)
      types::share_type speed = 0;
      /// The position of this producer when we last updated the records
      types::uint128 position = 0;
      /// The "race time" when we last updated the records
      types::uint128 position_update_time = 0;
      /// The projected "race time" at which this producer will finish the race
      types::uint128 projected_finish_time = std::numeric_limits<types::uint128>::max();

      /// Set all fields on race, given the current speed, position, and time
      void update(types::share_type currentSpeed, types::uint128 current_position, types::uint128 current_race_time) {
         speed = currentSpeed;
         position = current_position;
         position_update_time = current_race_time;
         auto distance_remaining = config::producer_race_lap_length - position;
         auto projected_time_to_finish = speed > 0? distance_remaining / speed
                                               : std::numeric_limits<types::uint128>::max();
         EOS_ASSERT(current_race_time <= std::numeric_limits<types::uint128>::max() - projected_time_to_finish,
                    producer_race_overflow_exception, "Producer race time has overflowed",
                    ("currentTime", current_race_time)("timeToFinish", projected_time_to_finish)("limit", std::numeric_limits<types::uint128>::max()));
         projected_finish_time = current_race_time + projected_time_to_finish;
      }
   } race;

   void start_new_race_lap(types::uint128 current_race_time) { race.update(race.speed, 0, current_race_time); }
   types::uint128 projected_race_finish_time() const { return race.projected_finish_time; }

   typedef std::pair<types::uint128,id_type> rft_order_type;
   rft_order_type projected_race_finish_time_order() const { return {race.projected_finish_time,id}; }
};

/**
 * @brief The proxy_vote_object tracks proxied votes
 * 
 * This object is created when an account indicates that it wishes to allow other accounts to proxy their votes to it,
 * so that the proxying accounts add their stake to the proxy target's votes.
 * 
 * When an account S proxies its votes to an account P, we look up the @ref proxy_vote_object with @ref proxy_target P,
 * and add S to the @ref proxy_sources, and add S's stake to @proxied_stake. We then update @ref proxy_target's votes, to
 * account for the change in its voting weight. Any time S's stake changes, we update the @ref proxied_stake accordingly.
 * If S terminates its vote delegation to P, we remove S and its stake from @ref proxySources and @ref proxiedStake, 
 * then update @ref proxy_target's votes. If P stops accepting proxied votes, then its @ref proxy_vote_object is removed
 * and all accounts listed in @ref proxy_sources revert back to an unproxied voting state.
 * 
 * Whenever any account A changes its votes, we check if there is some @ref ProxyVoteObject for which A is the target,
 * and if so, we add the @ref proxiedStake to its voting weight.
 * 
 * An account A may only proxy to one account at a time, and if A has proxied its votes to some other account, A may 
 * not cast any other votes until it unproxies its voting power.
 */
class proxy_vote_object : public chainbase::object<proxy_vote_object_type, proxy_vote_object> {
   OBJECT_CTOR(proxy_vote_object, (proxy_sources))

   id_type id;
   /// The account receiving the proxied voting power
   types::account_name proxy_target;
   /// The list of accounts proxying their voting power to @ref proxy_target
   shared_set<types::account_name> proxy_sources;
   /// The total stake proxied to @ref proxy_target. At all times, this should be equal to the sum of stake over all
   /// accounts in @ref proxy_sources
   types::share_type proxied_stake = 0;
   
   void add_proxy_source(const types::account_name& source, share_type source_stake, chainbase::database& db) const;
   void removeProxySource(const types::account_name& source, share_type source_stake,
                          chainbase::database& db) const;
   void update_proxied_stake(share_type stake_delta, chainbase::database& db) const;

   /// Cancel proxying votes to @ref proxyTarget for all @ref proxySources
   void cancel_proxies(chainbase::database& db) const;
};

/**
 * @brief The producer_schedule_object class schedules producers into rounds
 *
 * This class stores the state of the virtual race to select runner-up producers, and provides the logic for selecting
 * a round of producers.
 *
 * This is a singleton object within the database; there will only be one stored.
 */
class producer_schedule_object : public chainbase::object<producer_schedule_object_type, producer_schedule_object> {
   OBJECT_CTOR(producer_schedule_object)

   id_type id;
   types::uint128 currentRaceTime = 0;

   /// Retrieve a reference to the producer_schedule_object stored in the provided database
   static const producer_schedule_object& get(const chainbase::database& db) { return db.get(id_type()); }

   /**
    * @brief Calculate the next round of block producers
    * @param db The blockchain database
    * @return The next round of block producers, sorted by owner name
    *
    * This method calculates the next round of block producers according to votes and the virtual race for runner-up
    * producers. Although it is a const method, it will use its non-const db parameter to update its own records, as
    * well as the race records stored in the @ref ProducerVotesObjects
    */
   producer_round calculate_next_round(chainbase::database& db) const;

   /**
    * @brief Reset all producers in the virtual race to the starting line, and reset virtual time to zero
    */
   void reset_producer_race(chainbase::database& db) const;
};

using boost::multi_index::const_mem_fun;
/// Index producers by their owner's name
struct by_owner_name;
/// Index producers by projected race finishing time, from soonest to latest
struct by_projected_race_finish_time;
/// Index producers by votes, from greatest to least
struct by_votes;

using producer_votes_multi_index = chainbase::shared_multi_index_container<
   producer_votes_object,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<producer_votes_object, producer_votes_object::id_type, &producer_votes_object::id>
      >,
      ordered_unique<tag<by_owner_name>,
         member<producer_votes_object, types::account_name, &producer_votes_object::ownerName>
      >,
      ordered_non_unique<tag<by_votes>,
         const_mem_fun<producer_votes_object, std::pair<types::share_type,producer_votes_object::id_type>, &producer_votes_object::get_vote_order>,
         std::greater< std::pair<types::share_type, producer_votes_object::id_type> >
       >,
      ordered_unique<tag<by_projected_race_finish_time>,
         const_mem_fun<producer_votes_object, producer_votes_object::rft_order_type, &producer_votes_object::projected_race_finish_time_order>
      >
/*
   For some reason this does not compile, so I simulate it by adding a new method
      ordered_unique<tag<by_votes>,
         composite_key<
            const_mem_fun<producer_votes_object, types::share_type, &producer_votes_object::get_votes>,
            member<producer_votes_object, producer_votes_object::id_type, &producer_votes_object::id>
         >,
         composite_key_compare< std::greater<types::share_type>, std::less<producer_votes_object::id_type> >
      >*//*,
      */
   >
>;

/// Index proxies by the proxy target account name
struct by_target_name;

using proxy_vote_multi_index = chainbase::shared_multi_index_container<
   proxy_vote_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<proxy_vote_object, proxy_vote_object::id_type, &proxy_vote_object::id>>,
      ordered_unique<tag<by_target_name>, member<proxy_vote_object, types::account_name, &proxy_vote_object::proxy_target>>
   >
>;

using producer_schedule_multi_index = chainbase::shared_multi_index_container<
   producer_schedule_object,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<producer_schedule_object, producer_schedule_object::id_type, &producer_schedule_object::id>
      >
   >
>;

} } // namespace eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::producer_votes_object, eosio::chain::producer_votes_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::proxy_vote_object, eosio::chain::proxy_vote_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::producer_schedule_object, eosio::chain::producer_schedule_multi_index)
