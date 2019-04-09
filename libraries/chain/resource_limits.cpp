#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/transaction.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <eosio/chain/database_utils.hpp>
#include <algorithm>
#include <eosio/chain/stake.hpp>
#include <eosio/chain/account_object.hpp>

namespace eosio { namespace chain { namespace resource_limits {
using namespace stake;

using resource_index_set = index_set<
   resource_usage_table,
   resource_limits_state_table,
   resource_limits_config_table
>;

static_assert( config::rate_limiting_precision > 0, "config::rate_limiting_precision must be positive" );

static uint64_t update_elastic_limit(uint64_t current_limit, uint64_t average_usage, const elastic_limit_parameters& params) {
   uint64_t result = current_limit;
   if (average_usage > params.target ) {
      result = result * params.contract_rate;
   } else {
      result = result * params.expand_rate;
   }
   return std::min(std::max(result, params.max), params.max * params.max_multiplier);
}

void elastic_limit_parameters::validate()const {
   // At the very least ensure parameters are not set to values that will cause divide by zero errors later on.
   // Stricter checks for sensible values can be added later.
   EOS_ASSERT( periods > 0, resource_limit_exception, "elastic limit parameter 'periods' cannot be zero" );
   EOS_ASSERT( contract_rate.denominator > 0, resource_limit_exception, "elastic limit parameter 'contract_rate' is not a well-defined ratio" );
   EOS_ASSERT( expand_rate.denominator > 0, resource_limit_exception, "elastic limit parameter 'expand_rate' is not a well-defined ratio" );
}


void resource_limits_state_object::update_virtual_cpu_limit( const resource_limits_config_object& cfg ) {
   //idump((average_block_cpu_usage.average()));
   virtual_cpu_limit = update_elastic_limit(virtual_cpu_limit, average_block_cpu_usage.average(), cfg.cpu_limit_parameters);
   //idump((virtual_cpu_limit));
}

void resource_limits_state_object::update_virtual_net_limit( const resource_limits_config_object& cfg ) {
   virtual_net_limit = update_elastic_limit(virtual_net_limit, average_block_net_usage.average(), cfg.net_limit_parameters);
}

void resource_limits_manager::add_indices() {
   resource_index_set::add_indices(_chaindb);
}

void resource_limits_manager::initialize_database() {
    auto& config = _chaindb.emplace<resource_limits_config_object>([](resource_limits_config_object& config){
      // see default settings in the declaration
   });
   _chaindb.emplace<resource_limits_state_object>([&config](resource_limits_state_object& state){
      // see default settings in the declaration

      // start the chain off in a way that it is "congested" aka slow-start
      state.virtual_cpu_limit = config.cpu_limit_parameters.max;
      state.virtual_net_limit = config.net_limit_parameters.max;
   });

}

void resource_limits_manager::add_to_snapshot( const snapshot_writer_ptr& snapshot ) const {
   // TODO: Removed by CyberWay
}

void resource_limits_manager::read_from_snapshot( const snapshot_reader_ptr& snapshot ) {
   // TODO: Removed by CyberWay
}

void resource_limits_manager::initialize_account(const account_name& account) {
   _chaindb.emplace<resource_usage_object>([&]( resource_usage_object& bu ) {
      bu.owner = account;
   });
}

void resource_limits_manager::set_block_parameters(const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters ) {
   cpu_limit_parameters.validate();
   net_limit_parameters.validate();
   const auto& config = _chaindb.get<resource_limits_config_object>();
   _chaindb.modify(config, [&](resource_limits_config_object& c){
      c.cpu_limit_parameters = cpu_limit_parameters;
      c.net_limit_parameters = net_limit_parameters;
   });
}

void resource_limits_manager::update_account_usage(const flat_set<account_name>& accounts, uint32_t time_slot ) {
   const auto& config = _chaindb.get<resource_limits_config_object>();
   auto usage_table = _chaindb.get_table<resource_usage_object>();
   auto owner_idx = usage_table.get_index<by_owner>();
   for( const auto& a : accounts ) {
      const auto& usage = owner_idx.get( a );
      usage_table.modify( usage, [&]( auto& bu ){
          bu.net_usage.add( 0, time_slot, config.account_net_usage_average_window );
          bu.cpu_usage.add( 0, time_slot, config.account_cpu_usage_average_window );
      });
   }
}


void resource_limits_manager::add_transaction_usage(const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, fc::time_point now) {
   auto state_table = _chaindb.get_table<resource_limits_state_object>();
   const auto& state = state_table.get();
   const auto& config = _chaindb.get<resource_limits_config_object>();
   auto usage_table = _chaindb.get_table<resource_usage_object>();
   auto owner_idx = usage_table.get_index<by_owner>();
   const pricelist& prices = get_pricelist();
   auto time_slot = block_timestamp_type(now).slot; 

   for( const auto& a : accounts ) {

      const auto& usage = owner_idx.get( a );

      usage_table.modify( usage, [&]( auto& bu ) {
          bu.net_usage.add( net_usage, time_slot, config.account_net_usage_average_window );
          bu.cpu_usage.add( cpu_usage, time_slot, config.account_cpu_usage_average_window );
      });
      EOS_ASSERT(get_account_balance(now.sec_since_epoch(), a, prices).stake >= 0, resource_exhausted_exception, 
             "authorizing account '${n}' has insufficient resources for this transaction", ("n", name(a))); 
   }

   // account for this transaction in the block and do not exceed those limits either
   state_table.modify(state, [&](resource_limits_state_object& rls){
      rls.pending_cpu_usage += cpu_usage;
      rls.pending_net_usage += net_usage;
   });

   EOS_ASSERT( state.pending_cpu_usage <= config.cpu_limit_parameters.max, block_resource_exhausted, "Block has insufficient cpu resources" );
   EOS_ASSERT( state.pending_net_usage <= config.net_limit_parameters.max, block_resource_exhausted, "Block has insufficient net resources" );
}

void resource_limits_manager::add_pending_ram_usage( const account_name account, int64_t ram_delta ) {
   if (ram_delta == 0) {
      return;
   }
   const auto& usage  = _chaindb.get<resource_usage_object,by_owner>( account );
   auto state_table = _chaindb.get_table<resource_limits_state_object>();
   const auto& state = state_table.get();

   EOS_ASSERT( ram_delta <= 0 || UINT64_MAX - usage.ram_usage >= (uint64_t)ram_delta, transaction_exception,
              "Ram usage delta would overflow UINT64_MAX");
   EOS_ASSERT(ram_delta >= 0 || usage.ram_usage >= (uint64_t)(-ram_delta), transaction_exception,
              "Ram usage delta would underflow UINT64_MAX");

   _chaindb.modify( usage, [&]( auto& u ) {
     u.ram_usage += ram_delta;
   });
   
   state_table.modify(state, [&](resource_limits_state_object& rls){
      rls.pending_ram_usage += ram_delta;
   });
}

std::map<symbol_code, uint64_t> resource_limits_manager::get_account_usage(const account_name& account)const {
    const auto& config = _chaindb.get<resource_limits_config_object>();
    auto usage_table = _chaindb.get_table<resource_usage_object>();
    auto usage_owner_idx = usage_table.get_index<by_owner>();
    const auto& usage = usage_owner_idx.get(account);
    
    std::map<symbol_code, uint64_t> ret;
    ret[cpu_code] = downgrade_cast<uint64_t>(integer_divide_ceil(
                            static_cast<uint128_t>(usage.cpu_usage.value_ex) * config.account_cpu_usage_average_window, 
                            static_cast<uint128_t>(config::rate_limiting_precision)));
    ret[net_code] = downgrade_cast<uint64_t>(integer_divide_ceil(
                            static_cast<uint128_t>(usage.net_usage.value_ex) * config.account_net_usage_average_window, 
                            static_cast<uint128_t>(config::rate_limiting_precision)));
    ret[ram_code] = usage.ram_usage;
    return ret;
}

void resource_limits_manager::process_block_usage(uint32_t block_num) {
   auto state_table = _chaindb.get_table<resource_limits_state_object>();
   const auto& s = state_table.get();
   const auto& config = _chaindb.get<resource_limits_config_object>();
   state_table.modify(s, [&](resource_limits_state_object& state){
      // apply pending usage, update virtual limits and reset the pending

      state.average_block_cpu_usage.add(state.pending_cpu_usage, block_num, config.cpu_limit_parameters.periods);
      state.update_virtual_cpu_limit(config);
      state.pending_cpu_usage = 0;

      state.average_block_net_usage.add(state.pending_net_usage, block_num, config.net_limit_parameters.periods);
      state.update_virtual_net_limit(config);
      state.pending_net_usage = 0;
      
      state.ram_usage += state.pending_ram_usage;
      state.pending_ram_usage = 0;
   });
}

uint64_t resource_limits_manager::get_virtual_block_cpu_limit() const {
   const auto& state = _chaindb.get<resource_limits_state_object>();
   return state.virtual_cpu_limit;
}

uint64_t resource_limits_manager::get_virtual_block_net_limit() const {
   const auto& state = _chaindb.get<resource_limits_state_object>();
   return state.virtual_net_limit;
}

uint64_t resource_limits_manager::get_block_cpu_limit() const {
   const auto& state = _chaindb.get<resource_limits_state_object>();
   const auto& config = _chaindb.get<resource_limits_config_object>();
   return config.cpu_limit_parameters.max - state.pending_cpu_usage;
}

uint64_t resource_limits_manager::get_block_net_limit() const {
   const auto& state = _chaindb.get<resource_limits_state_object>();
   const auto& config = _chaindb.get<resource_limits_config_object>();
   return config.net_limit_parameters.max - state.pending_net_usage;
}

pricelist resource_limits_manager::get_pricelist() const {
    
    struct resource_stat {
        symbol_code code;
        uint64_t used_pct;
        uint64_t capacity;
    };
    const auto& state  = _chaindb.get<resource_limits_state_object>();

    std::array<resource_stat, 3> res_stats = {{
        {
            .code = cpu_code,
            .used_pct = safe_share_to_pct(state.average_block_cpu_usage.average(), state.virtual_cpu_limit),
            .capacity = state.virtual_cpu_limit
        },{
            .code = net_code,
            .used_pct = safe_share_to_pct(state.average_block_net_usage.average(), state.virtual_net_limit),
            .capacity = state.virtual_net_limit
        },{
            .code = ram_code,
            .used_pct = safe_share_to_pct(state.ram_usage, state.virtual_ram_limit),
            .capacity = state.virtual_ram_limit
        }
    }};
    
    pricelist ret;
    uint64_t used_pct_sum = 0;
    for (auto& r : res_stats) {
        r.used_pct = std::max(config::min_resource_usage_pct, r.used_pct);
        used_pct_sum += r.used_pct;
        ret[r.code] = ratio{0ll, 1ll};
    }
    
    symbol_code token_code { symbol(CORE_SYMBOL).to_symbol_code() };
    const stake_param_object* param = nullptr;
    const stake_stat_object* stat = nullptr;
    
    if ((param = _chaindb.find<stake_param_object, by_id>(token_code.value)) && 
        (stat  = _chaindb.find<stake_stat_object, by_id>(token_code.value)) && stat->enabled && stat->total_staked != 0) {
        EOS_ASSERT(stat->total_staked > 0, chain_exception, "SYSTEM: incorrect total_staked");
        for (auto& r : res_stats) {
            ret[r.code] = ratio{
                safe_prop<uint64_t>(stat->total_staked, r.used_pct, used_pct_sum), 
                r.capacity
            };
        }
    }
    
    return ret;
}

account_balance resource_limits_manager::get_account_balance(int64_t now, const account_name& account, const pricelist& prices) {

    static constexpr account_balance no_limits{UINT64_MAX, UINT64_MAX};
    symbol_code token_code { symbol(CORE_SYMBOL).to_symbol_code() };
    
    const stake_param_object* param = nullptr;
    const stake_stat_object* stat = nullptr;
    
    if (_chaindb.get<account_object, by_name>(account).privileged || //assignments:
        !(param = _chaindb.find<stake_param_object, by_id>(token_code.value)) || 
        !(stat  = _chaindb.find<stake_stat_object, by_id>(token_code.value)) || 
        !stat->enabled || stat->total_staked == 0) {
            
        return no_limits;
    }
    
    EOS_ASSERT(stat->total_staked > 0, chain_exception, "SYSTEM: incorrect total_staked");
    
    auto agents_table = _chaindb.get_table<stake_agent_object>();
    auto agents_idx = agents_table.get_index<stake_agent_object::by_key>();
    
    uint64_t staked = 0;
    auto agent = agents_idx.find(agent_key(token_code, account));
    if (agent != agents_idx.end()) {
        ram_payer_info ram(*this, account);
        update_proxied(_chaindb, ram, now, token_code, account, param->frame_length, false); 

        auto total_funds = agent->get_total_funds();
        EOS_ASSERT(total_funds >= 0, chain_exception, "SYSTEM: incorrect total_funds value");
        EOS_ASSERT(agent->own_share >= 0, chain_exception, "SYSTEM: incorrect own_share value");
        EOS_ASSERT(agent->shares_sum >= 0, chain_exception, "SYSTEM: incorrect shares_sum value");
        staked = safe_prop<uint64_t>(total_funds, agent->own_share, agent->shares_sum);
    }
    
    const auto& state  = _chaindb.get<resource_limits_state_object>();
    auto max_ram_usage = safe_prop<uint64_t>(state.virtual_ram_limit, staked, stat->total_staked);
    auto res_usage = get_account_usage(account);
    
    EOS_ASSERT(max_ram_usage >= res_usage[ram_code], ram_usage_exceeded,
        "account ${a} has insufficient staked tokens (${s}) for use ram.\n ram usage ${ur};\n total staked ${ts};\n total ram ${tr};\n max ram usage ${mr}", 
        ("a", account)("s",staked)("ur", res_usage[ram_code])("ts", stat->total_staked)("tr", state.virtual_ram_limit)("mr", max_ram_usage));
    
    uint64_t cost = 0;
    for (auto& u : res_usage) {
        auto price_itr = prices.find(u.first);
        EOS_ASSERT(price_itr != prices.end(), transaction_exception, "SYSTEM: resource does not exist");
        auto add = safe_prop(u.second, price_itr->second.numerator, price_itr->second.denominator);
        cost = (UINT64_MAX - cost) > add ? cost + add : UINT64_MAX;
    }
    
    EOS_ASSERT(staked >= cost, resource_exhausted_exception, 
        "account ${a} has insufficient staked tokens (${s}).\n usage: cpu ${uc}, net ${un}, ram ${ur}; \n prices: cpu ${pc}, net ${pn}, ram ${pr};\n cost ${c}", 
        ("a", account)("s",staked)
        ("uc", res_usage[cpu_code])          ("un", res_usage[net_code])          ("ur", res_usage[ram_code])
        ("pc", prices.find(cpu_code)->second)("pn", prices.find(net_code)->second)("pr", prices.find(ram_code)->second)
        ("c", cost));

    return account_balance {
        .stake = staked - cost,
        .ram =   max_ram_usage - res_usage[ram_code]
    };
}

} } } /// eosio::chain::resource_limits
