#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/transaction.hpp>
#include <algorithm>

namespace eosio { namespace chain { namespace resource_limits {

static uint64_t update_elastic_limit(uint64_t current_limit, uint64_t average_usage, const elastic_limit_parameters& params) {
   uint64_t result = current_limit;
   if (average_usage > params.target ) {
      result = result * params.contract_rate;
   } else {
      result = result * params.expand_rate;
   }

   return std::min(std::max(result, params.max), params.max * params.max_multiplier);
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
   const auto& config = _db.create<resource_limits_config_object>([](resource_limits_config_object& config){
      // see default settings in the declaration
   });

   _db.create<resource_limits_state_object>([&config](resource_limits_state_object& state){
      // see default settings in the declaration

      // start the chain off in a way that it is "congested" aka slow-start
      state.virtual_cpu_limit = config.cpu_limit_parameters.max;
      state.virtual_net_limit = config.net_limit_parameters.max;
   });
}

void resource_limits_manager::initialize_account(const account_name& account) {
   _db.create<resource_limits_object>([&]( resource_limits_object& bl ) {
      bl.owner = account;
   });

   _db.create<resource_usage_object>([&]( resource_usage_object& bu ) {
      bu.owner = account;
   });
}

void resource_limits_manager::set_block_parameters(const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters ) {
   const auto& config = _db.get<resource_limits_config_object>();
   _db.modify(config, [&](resource_limits_config_object& c){
      c.cpu_limit_parameters = cpu_limit_parameters;
      c.net_limit_parameters = net_limit_parameters;
   });
}

void resource_limits_manager::add_transaction_usage(const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t time_slot ) {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   set<std::pair<account_name, permission_name>> authorizing_accounts;

   for( const auto& a : accounts ) {

      const auto& usage = _db.get<resource_usage_object,by_owner>( a );
      const auto& limits = _db.get<resource_limits_object,by_owner>( boost::make_tuple(false, a));
      _db.modify( usage, [&]( auto& bu ){
          bu.net_usage.add( net_usage, time_slot, config.net_limit_parameters.periods );
          bu.cpu_usage.add( cpu_usage, time_slot, config.cpu_limit_parameters.periods );
      });

      if (limits.cpu_weight >= 0) {
         uint128_t  consumed_cpu_ex = usage.cpu_usage.consumed * config::rate_limiting_precision;
         uint128_t  capacity_cpu_ex = state.virtual_cpu_limit * config::rate_limiting_precision;

         EOS_ASSERT( state.total_cpu_weight > 0 && (consumed_cpu_ex * state.total_cpu_weight) <= (limits.cpu_weight * capacity_cpu_ex),
                     tx_resource_exhausted,
                     "authorizing account '${n}' has insufficient cpu resources for this transaction",
                     ("n",                    name(a))
                     ("consumed",             (double)consumed_cpu_ex/(double)config::rate_limiting_precision)
                     ("cpu_weight",           limits.cpu_weight)
                     ("virtual_cpu_capacity", (double)state.virtual_cpu_limit )
                     ("total_cpu_weight",     state.total_cpu_weight)
         );
      }

      if (limits.net_weight >= 0) {
         uint128_t  consumed_net_ex = usage.net_usage.consumed * config::rate_limiting_precision;
         uint128_t  capacity_net_ex = state.virtual_net_limit * config::rate_limiting_precision;

         EOS_ASSERT( state.total_net_weight > 0 && (consumed_net_ex * state.total_net_weight) <= (limits.net_weight * capacity_net_ex),
                     tx_resource_exhausted,
                     "authorizing account '${n}' has insufficient net resources for this transaction",
                     ("n",                    name(a))
                     ("consumed",             (double)consumed_net_ex/(double)config::rate_limiting_precision)
                     ("net_weight",           limits.net_weight)
                     ("virtual_net_capacity", (double)state.virtual_net_limit )
                     ("total_net_weight",     state.total_net_weight)
         );
      }
   }

   // account for this transaction in the block and do not exceed those limits either
   _db.modify(state, [&](resource_limits_state_object& rls){
      rls.pending_cpu_usage += cpu_usage;
      rls.pending_net_usage += net_usage;
   });

   EOS_ASSERT( state.pending_cpu_usage <= config.cpu_limit_parameters.max, block_resource_exhausted, "Block has insufficient cpu resources" );
   EOS_ASSERT( state.pending_net_usage <= config.net_limit_parameters.max, block_resource_exhausted, "Block has insufficient net resources" );
}

void resource_limits_manager::add_pending_account_ram_usage( const account_name account, int64_t ram_delta ) {
   if (ram_delta == 0) {
      return;
   }

   const auto& usage = _db.get<resource_usage_object,by_owner>( account );

   EOS_ASSERT(ram_delta < 0 || UINT64_MAX - usage.pending_ram_usage >= (uint64_t)ram_delta, transaction_exception, "Ram usage delta would overflow UINT64_MAX");
   EOS_ASSERT(ram_delta > 0 || usage.pending_ram_usage >= (uint64_t)(-ram_delta), transaction_exception, "Ram usage delta would underflow UINT64_MAX");

   _db.modify(usage, [&](resource_usage_object& o){
      o.pending_ram_usage += ram_delta;
   });
}

void resource_limits_manager::synchronize_account_ram_usage( ) {
   auto& multi_index = _db.get_mutable_index<resource_usage_index>();
   auto& by_dirty_index = multi_index.indices().get<by_dirty>();

   while(!by_dirty_index.empty()) {
      const auto& itr = by_dirty_index.lower_bound(boost::make_tuple(true));
      if (itr == by_dirty_index.end() || itr->is_dirty() != true) {
         break;
      }

      const auto& limits = _db.get<resource_limits_object,by_owner>( boost::make_tuple(false, itr->owner));

      if (limits.ram_bytes >= 0 && itr->pending_ram_usage > limits.ram_bytes) {
         tx_resource_exhausted e(FC_LOG_MESSAGE(error, "account ${a} has insufficient ram bytes", ("a", itr->owner)));
         e.append_log(FC_LOG_MESSAGE(error, "needs ${d} has ${m}", ("d",itr->pending_ram_usage)("m",limits.ram_bytes)));
         throw e;
      }

      _db.modify(*itr, [&](resource_usage_object& o){
         o.ram_usage = o.pending_ram_usage;
      });
   }
}

void resource_limits_manager::set_account_limits( const account_name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight) {
   const auto& usage = _db.get<resource_usage_object,by_owner>( account );
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

   if (ram_bytes >= 0) {
      if (limits.ram_bytes < 0 ) {
         EOS_ASSERT(ram_bytes >= usage.ram_usage, wasm_execution_error, "converting unlimited account would result in overcommitment [commit=${c}, desired limit=${l}]", ("c", usage.ram_usage)("l", ram_bytes));
      } else {
         EOS_ASSERT(ram_bytes >= usage.ram_usage, wasm_execution_error, "attempting to release committed ram resources [commit=${c}, desired limit=${l}]", ("c", usage.ram_usage)("l", ram_bytes));
      }

   }

   auto old_ram_bytes = limits.ram_bytes;
   _db.modify( limits, [&]( resource_limits_object& pending_limits ){
      pending_limits.ram_bytes = ram_bytes;
      pending_limits.net_weight = net_weight;
      pending_limits.cpu_weight = cpu_weight;
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


void resource_limits_manager::process_account_limit_updates() {
   auto& multi_index = _db.get_mutable_index<resource_limits_index>();
   auto& by_owner_index = multi_index.indices().get<by_owner>();

   // convenience local lambda to reduce clutter
   auto update_state_and_value = [](uint64_t &total, int64_t &value, int64_t pending_value, const char* debug_which) -> void {
      if (value > 0) {
         EOS_ASSERT(total >= value, rate_limiting_state_inconsistent, "underflow when reverting old value to ${which}", ("which", debug_which));
         total -= value;
      }

      if (pending_value > 0) {
         EOS_ASSERT(UINT64_MAX - total >= pending_value, rate_limiting_state_inconsistent, "overflow when applying new value to ${which}", ("which", debug_which));
         total += pending_value;
      }

      value = pending_value;
   };

   const auto& state = _db.get<resource_limits_state_object>();
   _db.modify(state, [&](resource_limits_state_object& rso){
      while(!by_owner_index.empty()) {
         const auto& itr = by_owner_index.lower_bound(boost::make_tuple(true));
         if (itr == by_owner_index.end() || itr->pending!= true) {
            break;
         }

         const auto& actual_entry = _db.get<resource_limits_object, by_owner>(boost::make_tuple(false, itr->owner));
         _db.modify(actual_entry, [&](resource_limits_object& rlo){
            update_state_and_value(rso.total_ram_bytes,  rlo.ram_bytes,  itr->ram_bytes, "ram_bytes");
            update_state_and_value(rso.total_cpu_weight, rlo.cpu_weight, itr->cpu_weight, "cpu_weight");
            update_state_and_value(rso.total_net_weight, rlo.net_weight, itr->net_weight, "net_weight");
         });

         multi_index.remove(*itr);
      }
   });
}

void resource_limits_manager::process_block_usage(uint32_t block_num) {
   const auto& s = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   _db.modify(s, [&](resource_limits_state_object& state){
      // apply pending usage, update virtual limits and reset the pending

      state.average_block_cpu_usage.add(state.pending_cpu_usage, block_num, config.cpu_limit_parameters.periods);
      state.update_virtual_cpu_limit(config);
      state.pending_cpu_usage = 0;

      state.average_block_net_usage.add(state.pending_net_usage, block_num, config.net_limit_parameters.periods);
      state.update_virtual_net_limit(config);
      state.pending_net_usage = 0;

   });

}

uint64_t resource_limits_manager::get_virtual_block_cpu_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   return state.virtual_cpu_limit;
}

uint64_t resource_limits_manager::get_virtual_block_net_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   return state.virtual_net_limit;
}

uint64_t resource_limits_manager::get_block_cpu_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   return config.cpu_limit_parameters.max - state.pending_cpu_usage;
}

uint64_t resource_limits_manager::get_block_net_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   return config.net_limit_parameters.max - state.pending_net_usage;
}

int64_t resource_limits_manager::get_account_cpu_limit( const account_name& name ) const {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& usage = _db.get<resource_usage_object, by_owner>(name);
   const auto& limits = _db.get<resource_limits_object, by_owner>(boost::make_tuple(false, name));
   if (limits.cpu_weight < 0) {
      return -1;
   }

   uint128_t consumed_ex = (uint128_t)usage.cpu_usage.consumed * (uint128_t)config::rate_limiting_precision;
   uint128_t virtual_capacity_ex = (uint128_t)state.virtual_cpu_limit * (uint128_t)config::rate_limiting_precision;
   uint128_t usable_capacity_ex = (uint128_t)(virtual_capacity_ex * limits.cpu_weight) / (uint128_t)state.total_cpu_weight;

   if (usable_capacity_ex < consumed_ex) {
      return 0;
   }

   return (int64_t)((usable_capacity_ex - consumed_ex) / (uint128_t)config::rate_limiting_precision);
}

int64_t resource_limits_manager::get_account_net_limit( const account_name& name ) const {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& usage = _db.get<resource_usage_object, by_owner>(name);
   const auto& limits = _db.get<resource_limits_object, by_owner>(boost::make_tuple(false, name));
   if (limits.net_weight < 0) {
      return -1;
   }

   uint128_t consumed_ex = (uint128_t)usage.net_usage.consumed * (uint128_t)config::rate_limiting_precision;
   uint128_t virtual_capacity_ex = (uint128_t)state.virtual_net_limit * (uint128_t)config::rate_limiting_precision;
   uint128_t usable_capacity_ex = (uint128_t)(virtual_capacity_ex * limits.net_weight) / (uint128_t)state.total_net_weight;

   if (usable_capacity_ex < consumed_ex) {
      return 0;
   }

   return (int64_t)((usable_capacity_ex - consumed_ex) / (uint128_t)config::rate_limiting_precision);

}


} } } /// eosio::chain::resource_limits
