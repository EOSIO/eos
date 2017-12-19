/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eos/chain/get_config.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/types.hpp>

namespace eosio { namespace chain {

fc::variant_object get_config()
{
   fc::mutable_variant_object result;

   result["key_prefix"] = config::key_prefix;
   result["default_block_interval_seconds"] = config::default_block_interval_seconds;
   result["MaxBlockSize"] = config::default_max_block_size;
   result["MaxSecondsUntilExpiration"] = config::default_max_trx_lifetime;
   result["ProducerCount"] = config::blocks_per_round;
   result["irreversible_threshold_percent"] = config::irreversible_threshold_percent;
   return result;
}

} } // eosio::chain
