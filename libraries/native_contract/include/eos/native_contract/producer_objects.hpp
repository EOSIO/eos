#pragma once

#include <eos/chain/types.hpp>
#include <eos/chain/multi_index_includes.hpp>

#include <eos/types/types.hpp>

#include <eos/utilities/exception_macros.hpp>

#include <chainbase/chainbase.hpp>

#include <boost/multi_index/mem_fun.hpp>

namespace eos {

FC_DECLARE_EXCEPTION(ProducerRaceOverflowException, 10000000, "Producer Virtual Race time has overflowed");

/**
 * @brief The ProducerVotesObject class tracks all votes for and by the block producers
 *
 * This class tracks the voting for block producers, as well as the virtual time 'race' to select the runner-up block
 * producer.
 *
 * This class also tracks the votes cast by block producers on various chain configuration options and key documents.
 */
class ProducerVotesObject : public chainbase::object<chain::producer_votes_object_type, ProducerVotesObject> {
   OBJECT_CTOR(ProducerVotesObject)

   id_type id;
   types::AccountName ownerName;

   /**
    * @brief Update the tally of votes for the producer, while maintaining virtual time accounting
    *
    * This function will update the producer's position in the race
    *
    * @param deltaVotes The change in votes since the last update
    * @param currentRaceTime The current "race time"
    */
   void updateVotes(types::ShareType deltaVotes, types::UInt128 currentRaceTime);
   /// @brief Get the number of votes this producer has received
   types::ShareType getVotes() const { return race.speed; }

   /**
    * These fields are used for the producer scheduling algorithm which uses a virtual race to ensure that runner-up
    * producers are given proportional time for producing blocks. Producers are constantly running a racetrack of
    * length config::ProducerRaceLapLength, at a speed equal to the number of votes they have received. Runner-up
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
    * @warning Do not update these values directly; use @ref updateVotes instead!
    */
   struct {
      /// The current speed for this producer (which is actually the total votes for the producer)
      types::ShareType speed = 0;
      /// The position of this producer when we last updated the records
      types::UInt128 position = 0;
      /// The "race time" when we last updated the records
      types::UInt128 positionUpdateTime = 0;
      /// The projected "race time" at which this producer will finish the race
      types::UInt128 projectedFinishTime = std::numeric_limits<types::UInt128>::max();

      /// Set all fields on race, given the current speed, position, and time
      void update(types::ShareType currentSpeed, types::UInt128 currentPosition, types::UInt128 currentRaceTime) {
         speed = currentSpeed;
         position = currentPosition;
         positionUpdateTime = currentRaceTime;
         auto distanceRemaining = config::ProducerRaceLapLength - position;
         auto projectedTimeToFinish = speed > 0? distanceRemaining / speed
                                               : std::numeric_limits<types::UInt128>::max();
         EOS_ASSERT(currentRaceTime <= std::numeric_limits<types::UInt128>::max() - projectedTimeToFinish,
                    ProducerRaceOverflowException, "Producer race time has overflowed",
                    ("currentTime", currentRaceTime)("timeToFinish", projectedTimeToFinish)("limit", std::numeric_limits<types::UInt128>::max()));
         projectedFinishTime = currentRaceTime + projectedTimeToFinish;
      }
   } race;

   void startNewRaceLap(types::UInt128 currentRaceTime) { race.update(race.speed, 0, currentRaceTime); }
   types::UInt128 projectedRaceFinishTime() const { return race.projectedFinishTime; }
};

/**
 * @brief The ProducerScheduleObject class schedules producers into rounds
 *
 * This class stores the state of the virtual race to select runner-up producers, and provides the logic for selecting
 * a round of producers.
 *
 * This is a singleton object within the database; there will only be one stored.
 */
class ProducerScheduleObject : public chainbase::object<chain::producer_schedule_object_type, ProducerScheduleObject> {
   OBJECT_CTOR(ProducerScheduleObject)

   id_type id;
   types::UInt128 currentRaceTime = 0;

   /// Retrieve a reference to the ProducerScheduleObject stored in the provided database
   static const ProducerScheduleObject& get(const chainbase::database& db) { return db.get(id_type()); }

   /**
    * @brief Calculate the next round of block producers
    * @param db The blockchain database
    * @return The next round of block producers, sorted by owner name
    *
    * This method calculates the next round of block producers according to votes and the virtual race for runner-up
    * producers. Although it is a const method, it will use its non-const db parameter to update its own records, as
    * well as the race records stored in the @ref ProducerVotesObjects
    */
   chain::ProducerRound calculateNextRound(chainbase::database& db) const;

   /**
    * @brief Reset all producers in the virtual race to the starting line, and reset virtual time to zero
    */
   void resetProducerRace(chainbase::database& db) const;
};

using boost::multi_index::const_mem_fun;
/// Index producers by their owner's name
struct byOwnerName;
/// Index producers by projected race finishing time, from soonest to latest
struct byProjectedRaceFinishTime;
/// Index producers by votes, from greatest to least
struct byVotes;

using ProducerVotesMultiIndex = chainbase::shared_multi_index_container<
   ProducerVotesObject,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<ProducerVotesObject, ProducerVotesObject::id_type, &ProducerVotesObject::id>
      >,
      ordered_unique<tag<byOwnerName>,
         member<ProducerVotesObject, types::AccountName, &ProducerVotesObject::ownerName>
      >,
      ordered_non_unique<tag<byVotes>,
         const_mem_fun<ProducerVotesObject, types::ShareType, &ProducerVotesObject::getVotes>,
         std::greater<types::ShareType>
      >,
      ordered_non_unique<tag<byProjectedRaceFinishTime>,
         const_mem_fun<ProducerVotesObject, types::UInt128, &ProducerVotesObject::projectedRaceFinishTime>
      >
   >
>;

using ProducerScheduleMultiIndex = chainbase::shared_multi_index_container<
   ProducerScheduleObject,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<ProducerScheduleObject, ProducerScheduleObject::id_type, &ProducerScheduleObject::id>
      >
   >
>;

} // namespace eos

CHAINBASE_SET_INDEX_TYPE(eos::ProducerVotesObject, eos::ProducerVotesMultiIndex)
CHAINBASE_SET_INDEX_TYPE(eos::ProducerScheduleObject, eos::ProducerScheduleMultiIndex)
