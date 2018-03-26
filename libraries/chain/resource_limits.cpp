#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/transaction.hpp>
#include <algorithm>

namespace eosio { namespace chain { namespace resource_limits {

static uint64_t update_elastic_limit(uint64_t current_limit, uint64_t average_usage_ex, const elastic_limit_parameters& params) {
   uint64_t result = current_limit;
   if (average_usage_ex > (params.target * config::rate_limiting_precision) ) {
      result = result * params.contract_rate;
   } else {
      result = result * params.expand_rate;
   }

   uint64_t min = params.max * params.periods;
   return std::min(std::max(result, min), min * params.max_multiplier);
}

void resource_limits_state_object::update_virtual_cpu_limit( const resource_limits_config_object& cfg ) {
   virtual_cpu_limit = update_elastic_limit(virtual_cpu_limit, average_block_cpu_usage.average(), cfg.cpu_limit_parameters);
}

void resource_limits_state_object::update_virtual_net_limit( const resource_limits_config_object& cfg ) {
   virtual_net_limit = update_elastic_limit(virtual_net_limit, average_block_net_usage.average(), cfg.net_limit_parameters);
}

void resource_limits_manager::initialize_database() {
   _db.add_index<resource_limits_index>();
   _db.add_index<resource_usage_index>();
   _db.add_index<resource_limits_state_index>();
   _db.add_index<resource_limits_config_index>();
}

void resource_limits_manager::initialize_chain() {
   _db.create<resource_limits_config_object>([](resource_limits_config_object& config){
      // see default settings in the declaration
   });

   _db.create<resource_limits_state_object>([](resource_limits_state_object& state){
      // see default settings in the declaration
   });
}

void resource_limits_manager::initialize_account(const account_name& account) {
   _db.create<resource_limits_object>([&]( resource_limits_object& bu ) {
      bu.owner = account;
   });
}

void resource_limits_manager::add_account_usage(const vector<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t block_num ) {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   set<std::pair<account_name, permission_name>> authorizing_accounts;

   for( const auto& a : accounts ) {

      const auto& usage = _db.get<resource_usage_object,by_owner>( a );
      const auto& limits = _db.get<resource_limits_object,by_owner>( boost::make_tuple(false, a));
      _db.modify( usage, [&]( auto& bu ){
          bu.net_usage.add( net_usage, block_num, config.net_limit_parameters.periods );
          bu.cpu_usage.add( cpu_usage, block_num, config.net_limit_parameters.periods );
      });

      uint128_t  consumed_cpu_ex = usage.cpu_usage.value; // this is pre-multiplied by config::rate_limiting_precision
      uint128_t  capacity_cpu_ex = state.virtual_cpu_limit * config::rate_limiting_precision;
      EOS_ASSERT( limits.cpu_weight < 0 || (consumed_cpu_ex * state.total_cpu_weight) <= (limits.cpu_weight * capacity_cpu_ex),
                  tx_resource_exhausted,
                  "authorizing account '${n}' has insufficient cpu resources for this transaction",
                  ("n",                    name(a))
                  ("consumed",             (double)consumed_cpu_ex/(double)config::rate_limiting_precision)
                  ("cpu_weight",           limits.cpu_weight)
                  ("virtual_cpu_capacity", (double)state.virtual_cpu_limit/(double)config::rate_limiting_precision )
                  ("total_cpu_weight",     state.total_cpu_weight)
      );

      uint128_t  consumed_net_ex = usage.net_usage.value; // this is pre-multiplied by config::rate_limiting_precision
      uint128_t  capacity_net_ex = state.virtual_net_limit * config::rate_limiting_precision;

      EOS_ASSERT( limits.net_weight < 0 || (consumed_net_ex * state.total_net_weight) <= (limits.net_weight * capacity_net_ex),
                  tx_resource_exhausted,
                  "authorizing account '${n}' has insufficient cpu resources for this transaction",
                  ("n",                    name(a))
                  ("consumed",             (double)consumed_net_ex/(double)config::rate_limiting_precision)
                  ("net_weight",           limits.net_weight)
                  ("virtual_net_capacity", (double)state.virtual_net_limit/(double)config::rate_limiting_precision )
                  ("total_net_weight",     state.total_net_weight)
      );
   }
}

void resource_limits_manager::add_block_usage( uint64_t cpu_usage, uint64_t net_usage, uint32_t block_num ) {
   const auto& s = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   _db.modify(s, [&](resource_limits_state_object& state){
      state.average_block_net_usage.add(net_usage, block_num, config.net_limit_parameters.periods);
      state.update_virtual_net_limit(config);

      state.average_block_cpu_usage.add(cpu_usage, block_num, config.cpu_limit_parameters.periods);
      state.update_virtual_cpu_limit(config);
   });
}

void resource_limits_manager::set_account_limits( const account_name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight) {
   const auto& usage = _db.get<resource_usage_object>();
   if (ram_bytes >= 0) {
      EOS_ASSERT(ram_bytes >= usage.ram_usage, wasm_execution_error, "attempting to release committed ram resources");
   }

   /*
    * Since we need to delay these until the next resource limiting boundary, these are created in a "pending"
    * state or adjusted in an existing "pending" state.  The chain controller will collapse "pending" state into
    * the actual state at the next appropriate boundary.
    */
   auto find_or_create_pending_limits = [&]() -> const resource_limits_object& {
      const auto* pending_limits = _db.find<resource_limits_object, by_owner>( boost::make_tuple(true, account) );
      if (pending_limits == nullptr) {
         const auto& limits = _db.get<resource_limits_object, by_owner>( boost::make_tuple(false, account));
         return _db.create<resource_limits_object>([&](resource_limits_object& pending_limits){
            pending_limits.owner = limits.owner;
            pending_limits.ram_bytes = limits.ram_bytes;
            pending_limits.net_weight = limits.net_weight;
            pending_limits.cpu_weight = limits.cpu_weight;
            pending_limits.pending = true;
         });
      } else {
         return *pending_limits;
      }
   };

   // update the users weights directly
   auto& limits = find_or_create_pending_limits();
   auto old_ram_bytes = limits.ram_bytes;
   _db.modify( limits, [&]( resource_limits_object& pending_limits ){
      pending_limits.ram_bytes = ram_bytes;
      pending_limits.net_weight = net_weight;
      pending_limits.cpu_weight = cpu_weight;
   });

   // TODO: make this thread safe when we need it
   // Currently, we have no parallel way of protecting the invariant that we don't overcommit the chain ram bytes
   // set via the config
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   auto new_speculative_ram_bytes = state.speculative_ram_bytes + ram_bytes - old_ram_bytes;
   EOS_ASSERT(new_speculative_ram_bytes <= config.max_ram_bytes, rate_limiting_overcommitment,
              "Allocating ${b} bytes of ram to account ${a} will overcommit configured maximum of ${m}",
              ("b", ram_bytes - old_ram_bytes)("a",account)("m",config.max_ram_bytes));
   _db.modify(state, [&](resource_limits_state_object &s ){
      s.speculative_ram_bytes = new_speculative_ram_bytes;
   });
}

void resource_limits_manager::get_account_limits( const account_name& account, int64_t& ram_bytes, int64_t& net_weight, int64_t& cpu_weight ) const {
   const auto* pending_buo = _db.find<resource_limits_object,by_owner>( boost::make_tuple(true, account) );
   if (pending_buo) {
      ram_bytes  = pending_buo->ram_bytes;
      net_weight = pending_buo->net_weight;
      cpu_weight = pending_buo->cpu_weight;
   } else {
      const auto& buo = _db.get<resource_limits_object,by_owner>( boost::make_tuple( false, account ) );
      ram_bytes  = buo.ram_bytes;
      net_weight = buo.net_weight;
      cpu_weight = buo.cpu_weight;
   }
}


void resource_limits_manager::process_pending_updates() {
   auto& multi_index = _db.get_mutable_index<resource_limits_index>();
   auto& by_owner_index = multi_index.indices().get<by_owner>();

   auto updated_state = _db.get<resource_limits_state_object>();

   while(!by_owner_index.empty()) {
      const auto& itr = by_owner_index.lower_bound(boost::make_tuple(true));
      if (itr == by_owner_index.end() || itr->pending!= true) {
         break;
      }

      const auto& actual_entry = _db.get<resource_limits_object, by_owner>(boost::make_tuple(false, itr->owner));
      _db.modify(actual_entry, [&](resource_limits_object& rlo){
         // convenience local lambda to reduce clutter
         auto update_state_and_value = [](uint64_t &total, int64_t &value, int64_t pending_value, const char* debug_which) -> void {
            if (value > 0) {
               EOS_ASSERT(total >= value, rate_limiting_state_inconsistent, "underflow when reverting old value to ${which}", ("which", debug_which));
               total -= value;
            }

            if (pending_value > 0) {
               EOS_ASSERT(UINT64_MAX - total < value, rate_limiting_state_inconsistent, "overflow when applying new value to ${which}", ("which", debug_which));
               total += pending_value;
            }

            value = pending_value;
         };

         update_state_and_value(updated_state.total_ram_bytes,  rlo.ram_bytes,  itr->ram_bytes, "ram_bytes");
         update_state_and_value(updated_state.total_cpu_weight, rlo.cpu_weight, itr->cpu_weight, "cpu_weight");
         update_state_and_value(updated_state.total_net_weight, rlo.net_weight, itr->net_weight, "net_wright");
      });

      multi_index.remove(*itr);
   }

   _db.modify(updated_state, [&updated_state](resource_limits_state_object rso){
      rso = updated_state;
   });
}

} } } /// eosio::chain::resource_limits
