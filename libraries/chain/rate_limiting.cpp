#include <eosio/chain/rate_limiting.hpp>
#include <algorithm>

namespace eosio { namespace chain {



static uint64_t update_elastic_limit(uint64_t current_limit, uint64_t average_usage_ex, const elastic_limit_parameters& params) {
   uint64_t result = current_limit;
   if (average_usage_ex > (params.target * config::rate_limiting_precision) ) {
      result = result * params.contract_rate;
   } else {
      result = result * params.expand_rate;
   }

   uint64_t min = params.max * params.periods;
   return std::min(std::max(result, min), min * params.max_multiplier));
}

void resource_limits_state_object::update_virtual_cpu_limit( const resource_limits_config_object& cfg ) {
   virtual_cpu_limit = update_elastic_limit(virtual_cpu_limit, average_block_cpu_usage.average(), cfg.cpu_limit_parameters);
}

void resource_limits_state_object::update_virtual_net_limit( const resource_limits_config_object& cfg ) {
   virtual_net_limit = update_elastic_limit(virtual_net_limit, average_block_net_usage.average(), cfg.net_limit_parameters);
}



} } /// eosio::chain
