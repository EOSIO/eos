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

} } } // namespace eosio::chain::contracts

CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::producer_votes_object, eosio::chain::contracts::producer_votes_multi_index)
