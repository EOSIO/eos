#pragma once

#include <eos/chain/types.hpp>

#include <eos/types/generated.hpp>

namespace eos {
namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct BlockchainConfiguration : public types::BlockchainConfiguration {
   using types::BlockchainConfiguration::BlockchainConfiguration;

   static BlockchainConfiguration get_median_values(std::vector<BlockchainConfiguration> votes);

   bool operator==(const BlockchainConfiguration& other) const;
   friend std::ostream& operator<< (std::ostream& s, const BlockchainConfiguration& p);
};

}
} // namespace eos::chain

FC_REFLECT_DERIVED(eos::chain::BlockchainConfiguration, (eos::types::BlockchainConfiguration), )
