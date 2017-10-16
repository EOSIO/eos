/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eos/chain/get_config.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/types.hpp>

namespace eos { namespace chain {

fc::variant_object get_config()
{
   fc::mutable_variant_object result;

   result["KeyPrefix"] = config::KeyPrefix;
   result["BlockIntervalSeconds"] = config::BlockIntervalSeconds;
   result["MaxBlockSize"] = config::DefaultMaxBlockSize;
   result["MaxSecondsUntilExpiration"] = config::DefaultMaxTrxLifetime;
   result["ProducerCount"] = config::BlocksPerRound;
   result["IrreversibleThresholdPercent"] = config::IrreversibleThresholdPercent;
   return result;
}

} } // eos::chain
