/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/types.hpp>




#include <eos/types/generated.hpp>

namespace eosio {
namespace chain {

/**
 * @brief Producer-voted blockchain configuration parameters
 *
 * This object stores the blockchain configuration, which is set by the block producers. Block producers each vote for
 * their preference for each of the parameters in this object, and the blockchain runs according to the median of the
 * values specified by the producers.
 */
struct blockchain_configuration : public types::blockchain_configuration {
   using types::blockchain_configuration::blockchain_configuration;

   blockchain_configuration& operator= (const types::blockchain_configuration& other);

   static blockchain_configuration get_median_values(std::vector<blockchain_configuration> votes);

   friend std::ostream& operator<< (std::ostream& s, const blockchain_configuration& p);
};

bool operator==(const types::blockchain_configuration& a, const types::blockchain_configuration& b);
inline bool operator!=(const types::blockchain_configuration& a, const types::blockchain_configuration& b) {
   return !(a == b);
}

}
} // namespace eosio::chain

FC_REFLECT_DERIVED(eosio::chain::blockchain_configuration, (eosio::types::blockchain_configuration), )
