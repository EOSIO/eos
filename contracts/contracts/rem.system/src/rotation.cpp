#include <rem.system/rem.system.hpp>


namespace eosiosystem {

/**
 * Rotation algorithm:
 *  top20 is never rotated
 *  until schedule is changed, every 4hours rotate each producer from top21-25
 *    first goes top21 replacing top21
 *    then goes top22 replacing top21
 *    and so on..
 *  if some producer neither from current schedule top21 nor from top25 reaches top21 position 
 *  we should not rotate him, so we reset rotation to current time point and schedule him for further rotations.
 */
std::vector<eosio::producer_key> system_contract::get_rotated_schedule() {
   auto sorted_prods = _producers.get_index<"prototalvote"_n>();

   std::vector<eosio::producer_key> top21_prods;
   top21_prods.reserve(max_block_producers);

   auto prod_it = std::begin(sorted_prods);
   for (; top21_prods.size() < max_block_producers
         && 0 < prod_it->total_votes && prod_it->active() && prod_it != std::end(sorted_prods); ++prod_it) {
      top21_prods.push_back( eosio::producer_key{ prod_it->owner, prod_it->producer_key } );
   }

   // nothing to rotate
   if (top21_prods.size() < max_block_producers) {
      return top21_prods;
   }

   const auto& to_out = top21_prods.back(); // candidate to be rotated

   // check if current top21 was in top21 in previous schedule
   const auto inTop21 = std::find_if(
      std::begin(_gstate.last_schedule),
      std::end(_gstate.last_schedule),
      [top21Name = to_out.producer_name]( const auto& prod ){ 
         return prod.first == top21Name;
      }
   );
   
   // check if current top21 was in top25 in previous schedule
   const auto inTop25 = std::find_if(
      std::begin(_grotation.standby_rotation),
      std::end(_grotation.standby_rotation),
      [top21Name = to_out.producer_name]( const auto& prod ){ 
         return prod.producer_name == top21Name;
      }
   );

   // if someone nor from top21 neither from top25 reached top21 then start rotation from now
   // and schedule top21 to be rotate in next schedules
   if ( inTop21 == std::end(_gstate.last_schedule) && inTop25 == std::end(_grotation.standby_rotation) ) {
      _grotation.last_rotation_time = eosio::current_time_point();
      _grotation.standby_rotation.push_back( to_out ); 

      return top21_prods;
   }

   // top 21-25
   std::vector<eosio::producer_key> standby;   
   for (prod_it = std::prev( prod_it ); standby.size() < _grotation.standby_prods_to_rotate + 1 // top21 + top22-25
         && 0 < prod_it->total_votes && prod_it->active() && prod_it != std::end(sorted_prods); ++prod_it) {
      standby.push_back( eosio::producer_key{ prod_it->owner, prod_it->producer_key } );
   }

   // still nothing to rotate
   if ( standby.size() <= 1 ) {
      return top21_prods;
   }

   std::vector<eosio::producer_key> rotation;

   // first go prods which were in previous rotation
   for(const auto& prev: _grotation.standby_rotation){
      auto it = std::find_if(
         std::begin(standby), std::end(standby),
         [&prev](const auto& value) {
            return value.producer_name == prev.producer_name;
         }
      ); 
      if( it != std::end(standby) ) {
         rotation.push_back(prev);
      }
   }

   // then go new prods
   for(const auto& prod: standby){
      auto it = std::find_if(
         std::begin(_grotation.standby_rotation), std::end(_grotation.standby_rotation),
         [&prod](const auto& value) {
            return value.producer_name == prod.producer_name;
         }
      ); 
      if( it == std::end(_grotation.standby_rotation) ) {
         rotation.push_back(prod);
      }
   }

   top21_prods.back() = rotation.front();

   const auto ct = eosio::current_time_point();
   const auto next_rotation_time = _grotation.last_rotation_time + _grotation.rotation_period;

   // rotation is done only once per 4 hours
   if (next_rotation_time <= ct) {
      std::rotate( std::begin(rotation), std::begin(rotation) + 1, std::end(rotation) );
      _grotation.last_rotation_time = ct;
      _grotation.standby_rotation   = rotation;
   }

   return top21_prods;
}

} /// namespace eosiosystem