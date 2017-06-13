#pragma once

#include <eos/chain/BlockchainConfiguration.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/message.hpp>
#include <eos/chain/config.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {
class chain_controller;

/**
 * The @ref chain_controller is not responsible for scheduling block producers, calculating the current @ref 
 * BlockchainConfiguration, but it needs to know the results of these calculations. This interface allows the @ref 
 * chain_controller to invoke these calculations and retrieve their results when needed.
 * 
 * @note This class uses the Curiously Recurring Template Pattern for static polymorphism: 
 * https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern
 */
class chain_administration_interface {
public:
   using ProducerRound = std::array<AccountName, config::ProducerCount>;

   virtual ~chain_administration_interface();

   virtual ProducerRound get_next_round(const chainbase::database& db) = 0;
   virtual BlockchainConfiguration get_blockchain_configuration(const chainbase::database& db,
                                                                const ProducerRound& round) = 0;
};

} } // namespace eos::chain
