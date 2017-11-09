/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/blockchain_configuration.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/message.hpp>
#include <eos/chain/config.hpp>

namespace chainbase { class database; }

namespace eosio { namespace chain {
class chain_controller;

/**
 * The @ref chain_controller is not responsible for scheduling block producers, calculating the current @ref 
 * BlockchainConfiguration, but it needs to know the results of these calculations. This interface allows the @ref 
 * chain_controller to invoke these calculations and retrieve their results when needed.
 */
class chain_administration_interface {
public:
   virtual ~chain_administration_interface();

   /**
    * @brief Calculate the next round of block producers and return it
    * @param db The current blockchain database state. Private state or block producer scheduling may be modiied.
    * @return The next round of block producers, sorted by owner name
    */
   virtual producer_round get_next_round(chainbase::database& db) = 0;
   virtual blockchain_configuration get_blockchain_configuration(const chainbase::database& db,
                                                                const producer_round& round) = 0;
};

} } // namespace eosio::chain
