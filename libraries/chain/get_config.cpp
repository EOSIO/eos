/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/get_config.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

fc::variant_object get_config()
{
   fc::mutable_variant_object result;

//   result["block_interval_ms"] = config::block_interval_ms;
//   result["producer_count"] = config::producer_count;
   /// TODO: add extra config parms
   return result;
}

} } // eosio::chain
