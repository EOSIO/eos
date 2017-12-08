/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/multi_index_includes.hpp>

#include <chainbase/chainbase.hpp>

#include <fc/static_variant.hpp>

namespace eosio { namespace chain { namespace contracts {

/**
 * @brief The producer_slate struct stores a list of producers voted on by an account
 */
struct producer_slate {
   std::array<account_name, config::max_producer_votes> votes;
   size_t size = 0;

   void add(account_name producer) {
      votes[size++] = producer;
      std::inplace_merge(votes.begin(), votes.begin() + size - 1, votes.begin() + size);
   }
   void remove(account_name producer) {
      auto itr = std::remove(votes.begin(), votes.begin() + size, producer);
      size = std::distance(votes.begin(), itr);
   }
   bool contains(account_name producer) const {
      return std::binary_search(votes.begin(), votes.begin() + size, producer);
   }

   auto range() { return boost::make_iterator_range_n(votes.begin(), size); }
   auto range() const { return boost::make_iterator_range_n(votes.begin(), size); }
};

/**
 * @brief The staked_balance_object class tracks the staked balance (voting balance) for accounts
 */
class staked_balance_object : public chainbase::object<staked_balance_object_type, staked_balance_object> {
   OBJECT_CTOR(staked_balance_object)

   id_type id;
   account_name owner_name;

   share_type staked_balance = 0;
   share_type unstaking_balance = 0;
   time_point last_unstaking_time = time_point::maximum();

   uint128_t   staked_balance_percent = 0;

   /// The account's vote on producers. This may either be a list of approved producers, or an account to proxy vote to
   fc::static_variant<producer_slate, account_name> producer_votes = producer_slate{};

   /**
    * @brief Add the provided stake to this balance, maintaining invariants
    * @param new_stake The new stake to add to the balance
    * @param db Read-write reference to the database
    *
    * This method will update this object with the new stake, while maintaining invariants around the stake balance,
    * such as by updating vote tallies
    */
   void stake_tokens(share_type new_stake, chainbase::database& db) const;
   /**
    * @brief Begin unstaking the specified amount of stake, maintaining invariants
    * @param amount The amount of stake to begin unstaking
    * @param db Read-write reference to the database
    *
    * This method will update this object's balances while maintaining invariants around the stake balances, such as by
    * updating vote tallies
    */
   void begin_unstaking_tokens(share_type amount, chainbase::database& db) const;
   /**
    * @brief Finish unstaking the specified amount of stake
    * @param amount The amount of stake to finish unstaking
    * @param db Read-write reference to the database
    *
    * This method will update this object's balances. There aren't really any invariants to maintain on this call, as
    * the tokens are already unstaked and removed from vote tallies, but it is provided for completeness' sake.
    */
   void finish_unstaking_tokens(share_type amount, chainbase::database& db) const;

   /**
    * @brief Propagate the specified change in stake to the producer votes or the proxy
    * @param staked_delta The change in stake
    * @param db Read-write reference to the database
    *
    * This method will apply the provided delta in voting stake to the next stage in the producer voting pipeline,
    * whether that be the producers in the slate, or the account the votes are proxied to.
    *
    * This method will *not* update this object in any way. It will not adjust @ref staked_balance, etc
    */
   void propagate_votes(share_type staked_delta, chainbase::database& db) const;
};

struct by_owner_name;

using staked_balance_multi_index = chainbase::shared_multi_index_container<
   staked_balance_object,
   indexed_by<
      ordered_unique<tag<by_id>,
         member<staked_balance_object, staked_balance_object::id_type, &staked_balance_object::id>
      >,
      ordered_unique<tag<by_owner_name>,
         member<staked_balance_object, account_name, &staked_balance_object::owner_name>
      >
   >
>;

} } } // namespace eosio::chain::contracts

CHAINBASE_SET_INDEX_TYPE(eosio::chain::contracts::staked_balance_object, eosio::chain::contracts::staked_balance_multi_index)
