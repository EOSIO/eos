/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

   void chain_config::validate()const {

      for (size_t i = 0; i < resource_limits::resources_num; i++) {
         EOS_ASSERT( min_virtual_limits[i] <= max_virtual_limits[i], action_validate_exception, "min virtual limit > max virtual limits" );
         EOS_ASSERT( max_transaction_usage[i] < max_block_usage[i], action_validate_exception, "max transaction usage must be less than max block usage" );
      }

      EOS_ASSERT( ram_size >= config::_GB, action_validate_exception,
                  "RAM size can't be less than 1 GB");
      EOS_ASSERT( reserved_ram_size * 2 <= ram_size, action_validate_exception,
                  "Reserved RAM size can't be less than half of RAM size");

      EOS_ASSERT( context_free_discount_net_usage_den > 0, action_validate_exception,
                  "net usage discount ratio for context free data cannot have a 0 denominator" );
      EOS_ASSERT( context_free_discount_net_usage_num <= context_free_discount_net_usage_den, action_validate_exception,
                  "net usage discount ratio for context free data cannot exceed 1" );

      EOS_ASSERT( min_transaction_cpu_usage <= max_transaction_usage[resource_limits::CPU], action_validate_exception,
                  "min transaction cpu usage cannot exceed max transaction cpu usage" );
      EOS_ASSERT( max_transaction_usage[resource_limits::CPU] < (max_block_usage[resource_limits::CPU] - min_transaction_cpu_usage), action_validate_exception,
                  "max transaction cpu usage must be at less than the difference between the max block cpu usage and the min transaction cpu usage" );
      
      EOS_ASSERT( min_transaction_ram_usage <= max_transaction_usage[resource_limits::RAM], action_validate_exception,
                  "min transaction ram usage cannot exceed max transaction ram usage" );
      EOS_ASSERT( max_transaction_usage[resource_limits::RAM] < (max_block_usage[resource_limits::RAM] - min_transaction_ram_usage), action_validate_exception,
                  "max transaction ram usage must be at less than the difference between the max block ram usage and the min transaction ram usage" );

      EOS_ASSERT( 1 <= max_authority_depth, action_validate_exception,
                  "max authority depth should be at least 1" );
}

} } // namespace eosio::chain
