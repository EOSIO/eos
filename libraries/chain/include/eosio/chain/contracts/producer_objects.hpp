/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/multi_index_includes.hpp>

#include <eos/utilities/exception_macros.hpp>

#include <chainbase/chainbase.hpp>

#include <boost/multi_index/mem_fun.hpp>

namespace eosio { namespace chain { namespace contracts {

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

   id_type         id;
   account_name    owner_name;
   share_type      votes = 0;

   /**
    * @brief Update the tally of votes for the producer
    * @param delta_votes The change in votes since the last update
    */
   void update_votes(share_type delta_votes) {
      votes += delta_votes;
   }
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
 * If S terminates its vote delegation to P, we remove S and its stake from @ref proxy_sources and @ref proxied_stake, 
 * then update @ref proxy_target's votes. If P stops accepting proxied votes, then its @ref proxy_vote_object is removed 
 * and all accounts listed in @ref proxy_sources revert back to an unproxied voting state.
 * 
 * Whenever any account A changes its votes, we check if there is some @ref proxy_vote_object for which A is the target,
 * and if so, we add the @ref proxied_stake to its voting weight.
 * 
 * An account A may only proxy to one account at a time, and if A has proxied its votes to some other account, A may 
 * not cast any other votes until it unproxies its voting power.
 */
class proxy_vote_object : public chainbase::object<proxy_vote_object_type, proxy_vote_object> {
   OBJECT_CTOR(proxy_vote_object, (proxy_sources))

   id_type id;
   /// The account receiving the proxied voting power
   account_name proxy_target;
   /// The list of accounts proxying their voting power to @ref proxy_target
   shared_set<account_name> proxy_sources;
   /// The total stake proxied to @ref proxy_target. At all times, this should be equal to the sum of stake over all 
   /// accounts in @ref proxy_sources
   share_type proxied_stake = 0;
   
   void add_proxy_source(const account_name& source, share_type source_stake, chainbase::database& db) const;
   void remove_proxy_source(const account_name& source, share_type source_stake,
                          chainbase::database& db) const;
   void update_proxied_state(share_type staked_delta, chainbase::database& db) const;

   /// Cancel proxying votes to @ref proxy_target for all @ref proxy_sources
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
   uint128_t current_race_time = 0;

   /// Retrieve a reference to the producer_schedule_object stored in the provided database
   static const producer_schedule_object& get(const chainbase::database& db) { return db.get(id_type()); }

   /**
    * @brief Calculate the next round of block producers
    * @param db The blockchain database
    * @return The next round of block producers, sorted by owner name
    *
    * This method calculates the next round of block producers according to votes and the virtual race for runner-up
    * producers. Although it is a const method, it will use its non-const db parameter to update its own records, as
    * well as the race records stored in the @ref producer_votes_objects
    */
   //TODO: did the concept of producer rounds get destroyed?
   //producer_round calculate_next_round(chainbase::database& db) const;

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
      ordered_unique< tag<by_id>,
         member<producer_votes_object, producer_votes_object::id_type, &producer_votes_object::id>
      >,
      ordered_unique< tag<by_owner_name>,
         member<producer_votes_object, account_name, &producer_votes_object::owner_name>
      >,
      ordered_unique< tag<by_votes>,
         composite_key<producer_votes_object,
            member<producer_votes_object, share_type, &producer_votes_object::votes>,
            member<producer_votes_object, producer_votes_object::id_type, &producer_votes_object::id>
         >,
         composite_key_compare< std::greater<share_type>,std::less<producer_votes_object::id_type> >
      >
   >
>;

/// Index proxies by the proxy target account name
struct by_target_name;

using proxy_vote_multi_index = chainbase::shared_multi_index_container<
   proxy_vote_object,
   indexed_by<
      ordered_unique<tag<by_id>, member<proxy_vote_object, proxy_vote_object::id_type, &proxy_vote_object::id>>,
      ordered_unique<tag<by_target_name>, member<proxy_vote_object, account_name, &proxy_vote_object::proxy_target>>
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

} } } // namespace eosio::chain::contracts

CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::producer_votes_object, eosio::chain::contracts::producer_votes_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::proxy_vote_object, eosio::chain::contracts::proxy_vote_multi_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::producer_schedule_object, eosio::chain::contracts::producer_schedule_multi_index)
