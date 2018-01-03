/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/staked_balance_objects.hpp>
#include <eosio/chain/contracts/producer_objects.hpp>

#include <eosio/chain/global_property_object.hpp>

#include <boost/range/algorithm/for_each.hpp>

namespace eosio { namespace chain { namespace contracts {

void staked_balance_object::stake_tokens(share_type new_stake, chainbase::database& db) const {
   // Update the staked balance
   db.modify(*this, [&new_stake](staked_balance_object& sbo) {
      sbo.staked_balance += new_stake;
   });

   db.modify( db.get<dynamic_global_property_object>(), [&]( auto& dgpo ){
      dgpo.total_staked_tokens += new_stake;
   });

   propagate_votes(new_stake, db);
}

void staked_balance_object::begin_unstaking_tokens(share_type amount, chainbase::database& db) const {
   // Remember there might be stake left over from a previous, uncompleted unstaking in unstaking_balance
   auto delta_stake = unstaking_balance - amount;

   // Update actual stake balance and invariants around it
   stake_tokens(delta_stake, db);
   // Update stats for unstaking process
   db.modify(*this, [&amount, &db](staked_balance_object& sbo) {
      sbo.unstaking_balance = amount;
      sbo.last_unstaking_time = db.get(dynamic_global_property_object::id_type()).time;
   });

}

void staked_balance_object::finish_unstaking_tokens(share_type amount, chainbase::database& db) const {
   db.modify(*this, [&amount](staked_balance_object& sbo) {
      sbo.unstaking_balance -= amount;
   });
   db.modify( db.get<dynamic_global_property_object>(), [&]( auto& dgpo ){
      dgpo.total_staked_tokens -= amount;
   });
}

void staked_balance_object::propagate_votes(share_type staked_delta, chainbase::database& db) const {
   if (producer_votes.contains<producer_slate>())
      // This account votes for producers directly; update their stakes
      boost::for_each(producer_votes.get<producer_slate>().range(), [&db, &staked_delta](const account_name& name) {
         db.modify(db.get<producer_votes_object, by_owner_name>(name), [&db, &staked_delta](producer_votes_object& pvo) {
            pvo.update_votes(staked_delta);
         });
      });
   else {
   }
}

} } } // namespace eosio::chain::contracts
