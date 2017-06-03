#include "staked_balance_objects.hpp"

namespace eos {
using namespace chain;
using namespace types;

void ProducerVotesObject::updateVotes(ShareType deltaVotes, UInt128 currentRaceTime) {
   auto timeSinceLastUpdate = currentRaceTime - race.positionUpdateTime;
   race.position += race.speed * timeSinceLastUpdate;
   race.positionUpdateTime = currentRaceTime;

   race.speed += deltaVotes;
   auto distanceRemaining = config::ProducerRaceLapLength - race.position;
   auto projectedTimeToFinish = distanceRemaining / race.speed;

   race.projectedFinishTime = currentRaceTime + projectedTimeToFinish;
}

} // namespace eos
