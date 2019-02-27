#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/transaction.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <eosio/chain/database_utils.hpp>
#include <algorithm>
#include <eosio/chain/stake_object.hpp>
#include <eosio/chain/account_object.hpp>

namespace eosio { namespace chain { namespace resource_limits {

using resource_index_set = index_set<
   resource_usage_index,
   resource_limits_state_index,
   resource_limits_config_index,
   stake_agent_index,
   stake_grant_index,
   stake_param_index,
   stake_stat_index
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

static int64_t safe_prop(int64_t arg, int64_t numer, int64_t denom) {
    return !arg || !numer ? 0 : impl::downgrade_cast<int64_t>((static_cast<int128_t>(arg) * numer) / denom);
}

static int64_t safe_pct(int64_t arg, int64_t total) {
    return safe_prop(arg, total, config::_100percent);
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
   resource_index_set::add_indices(_db, _chaindb);
}

void resource_limits_manager::initialize_database() {
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

void resource_limits_manager::add_to_snapshot( const snapshot_writer_ptr& snapshot ) const {
   resource_index_set::walk_indices([this, &snapshot]( auto utils ){
      snapshot->write_section<typename decltype(utils)::index_t::value_type>([this]( auto& section ){
         decltype(utils)::walk(_db, [this, &section]( const auto &row ) {
            section.add_row(row, _db);
         });
      });
   });
}

void resource_limits_manager::read_from_snapshot( const snapshot_reader_ptr& snapshot ) {
   resource_index_set::walk_indices([this, &snapshot]( auto utils ){
      snapshot->read_section<typename decltype(utils)::index_t::value_type>([this]( auto& section ) {
         bool more = !section.empty();
         while(more) {
            decltype(utils)::create(_db, [this, &section, &more]( auto &row ) {
               more = section.read_row(row, _db);
            });
         }
      });
   });
}

void resource_limits_manager::initialize_account(const account_name& account) {
   _db.create<resource_usage_object>([&]( resource_usage_object& bu ) {
      bu.owner = account;
   });
}

void resource_limits_manager::set_block_parameters(const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters ) {
   cpu_limit_parameters.validate();
   net_limit_parameters.validate();
   const auto& config = _db.get<resource_limits_config_object>();
   _db.modify(config, [&](resource_limits_config_object& c){
      c.cpu_limit_parameters = cpu_limit_parameters;
      c.net_limit_parameters = net_limit_parameters;
   });
}

void resource_limits_manager::update_account_usage(const flat_set<account_name>& accounts, uint32_t time_slot ) {
   const auto& config = _db.get<resource_limits_config_object>();
   for( const auto& a : accounts ) {
      const auto& usage = _db.get<resource_usage_object,by_owner>( a );
      _db.modify( usage, [&]( auto& bu ){
          bu.net_usage.add( 0, time_slot, config.account_net_usage_average_window );
          bu.cpu_usage.add( 0, time_slot, config.account_cpu_usage_average_window );
      });
   }
}

void resource_limits_manager::add_transaction_usage(const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t time_slot, int64_t now) {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();

   for( const auto& a : accounts ) {

      const auto& usage = _db.get<resource_usage_object,by_owner>( a );

      _db.modify( usage, [&]( auto& bu ){
          bu.net_usage.add( net_usage, time_slot, config.account_net_usage_average_window );
          bu.cpu_usage.add( cpu_usage, time_slot, config.account_cpu_usage_average_window );
      });
      
      if (now >= 0) {
          auto net_limit_ex = get_account_limit_ex(now, a, resource_limits::net_code);
          EOS_ASSERT(net_limit_ex.used <= net_limit_ex.max, tx_net_usage_exceeded, 
             "authorizing account '${n}' has insufficient net resources for this transaction", ("n", name(a))); 
          
          auto cpu_limit_ex = get_account_limit_ex(now, a, resource_limits::cpu_code);
          EOS_ASSERT(cpu_limit_ex.used <= cpu_limit_ex.max, tx_cpu_usage_exceeded, 
             "authorizing account '${n}' has insufficient cpu resources for this transaction", ("n", name(a)));
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

void resource_limits_manager::add_pending_ram_usage( const account_name account, int64_t ram_delta ) {
   if (ram_delta == 0) {
      return;
   }
   const auto& usage  = _db.get<resource_usage_object,by_owner>( account );

   EOS_ASSERT( ram_delta <= 0 || UINT64_MAX - usage.ram_usage >= (uint64_t)ram_delta, transaction_exception,
              "Ram usage delta would overflow UINT64_MAX");
   EOS_ASSERT(ram_delta >= 0 || usage.ram_usage >= (uint64_t)(-ram_delta), transaction_exception,
              "Ram usage delta would underflow UINT64_MAX");

   _db.modify( usage, [&]( auto& u ) {
     u.ram_usage += ram_delta;
   });
}

int64_t resource_limits_manager::get_account_ram_usage( const account_name& name )const {
   return _db.get<resource_usage_object,by_owner>( name ).ram_usage;
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

account_resource_limit resource_limits_manager::get_account_limit_ex(int64_t now, const account_name& account, symbol_code purpose_code) const{
        
    static constexpr account_resource_limit no_limits{-1, -1, -1};
    
    EOS_ASSERT(purpose_code == cpu_code || purpose_code == net_code || purpose_code == ram_code, chain_exception,
        "SYSTEM: incorrect purpose_code");
    
    symbol_code token_code { symbol(CORE_SYMBOL).to_symbol_code() };
    
    const stake_param_object* param = nullptr;
    const stake_stat_object* stat = nullptr;
    
    if (_db.get<account_object, by_name>(account).privileged || //assignments:
        !(param = _db.find<stake_param_object, by_id>(token_code.value)) || 
        !(stat  = _db.find<stake_stat_object, stake_stat_object::by_key>(stat_key(purpose_code, token_code))) || 
        !stat->enabled || stat->total_staked == 0) {
            
        return no_limits;
    }
    
    EOS_ASSERT(stat->total_staked > 0, chain_exception, "SYSTEM: incorrect total_staked");

    const auto& agents_idx = _db.get_mutable_index<stake_agent_index>().indices().get<stake_agent_object::by_key>();
    
    int64_t staked = 0;
    auto agent = agents_idx.find(agent_key(purpose_code, token_code, account));
    if (agent != agents_idx.end()) {
        const auto& grants_idx = _db.get_mutable_index<stake_grant_index>().indices().get<stake_grant_object::by_key>();
        update_proxied_traversal(now, purpose_code, token_code, agents_idx, grants_idx, &*agent, param->frame_length, false);
        auto total_funds = agent->get_total_funds();
        staked = safe_prop(total_funds, agent->own_share, agent->shares_sum);
        EOS_ASSERT(staked >= 0, chain_exception, "SYSTEM: incorrect staked value");
    }
    const auto& config = _db.get<resource_limits_config_object>();
    const auto& state  = _db.get<resource_limits_state_object>();
    const auto& usage  = _db.get<resource_usage_object, by_owner>(account);
    int64_t capacity = 0;
    int64_t used = 0;
    if (purpose_code == cpu_code || purpose_code == net_code) {
        bool cpu = (purpose_code == cpu_code);
        int128_t window_size = cpu ? config.account_cpu_usage_average_window : config.account_net_usage_average_window;
        
        EOS_ASSERT((cpu ? state.virtual_cpu_limit : state.virtual_net_limit) <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()), 
            chain_exception, cpu ? "SYSTEM: incorrect virtual_cpu_limit" : "SYSTEM: incorrect virtual_net_limit");
        capacity = static_cast<int64_t>(cpu ? state.virtual_cpu_limit : state.virtual_net_limit);
        
        used = impl::downgrade_cast<int64_t>(impl::integer_divide_ceil(
            static_cast<int128_t>(cpu ? usage.cpu_usage.value_ex : usage.net_usage.value_ex) * window_size, 
            static_cast<int128_t>(config::rate_limiting_precision)));
    }
    else {
        EOS_ASSERT(state.virtual_ram_limit <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()), 
            chain_exception, "SYSTEM: incorrect virtual_ram_limit");
        EOS_ASSERT(usage.ram_usage <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()), 
            chain_exception, "SYSTEM: incorrect ram_usage");
        capacity = static_cast<int64_t>(state.virtual_ram_limit);
        used = static_cast<int64_t>(usage.ram_usage);
    }
    auto max_use = safe_prop(capacity, staked, stat->total_staked);
     
    int64_t diff_use = max_use - used;
    return account_resource_limit {
        .used = used,
        .available = std::max(diff_use, int64_t(0)),
        .max = max_use,
        .staked_virtual_balance = safe_prop(stat->total_staked, diff_use, capacity)
    };
}

void resource_limits_manager::update_proxied(int64_t now, symbol_code purpose_code, symbol_code token_code, const account_name& account, int64_t frame_length, bool force) {
    const auto& agents_idx = _db.get_mutable_index<stake_agent_index>().indices().get<stake_agent_object::by_key>();
    const auto& grants_idx = _db.get_mutable_index<stake_grant_index>().indices().get<stake_grant_object::by_key>();
    update_proxied_traversal(now, purpose_code, token_code, agents_idx, grants_idx, get_agent(purpose_code, token_code, agents_idx, account), 
        frame_length, force);
}

void resource_limits_manager::recall_proxied(int64_t now, symbol_code purpose_code, symbol_code token_code, 
                                            account_name grantor_name, account_name agent_name, int16_t pct) {
                        
    EOS_ASSERT(1 <= pct && pct <= config::_100percent, transaction_exception, "pct must be between 0.01% and 100% (1-10000)");
    const auto* param = _db.find<stake_param_object, by_id>(token_code.value);
    EOS_ASSERT(param, transaction_exception, "no staking for token");
    EOS_ASSERT(std::find(param->purposes.begin(), param->purposes.end(), purpose_code) != param->purposes.end(), transaction_exception, "unknown purpose");
    
    const auto& agents_idx = _db.get_mutable_index<stake_agent_index>().indices().get<stake_agent_object::by_key>();
    const auto& grants_idx = _db.get_mutable_index<stake_grant_index>().indices().get<stake_grant_object::by_key>();

    auto grantor_as_agent = get_agent(purpose_code, token_code, agents_idx, grantor_name);
    
    update_proxied_traversal(now, purpose_code, token_code, agents_idx, grants_idx, grantor_as_agent, param->frame_length, false);
    
    int64_t amount = 0;
    auto grant_itr = grants_idx.lower_bound(grant_key(purpose_code, token_code, grantor_name));
    while ((grant_itr != grants_idx.end()) &&
           (grant_itr->purpose_code   == purpose_code) &&
           (grant_itr->token_code   == token_code) &&
           (grant_itr->grantor_name == grantor_name))
    {
        if (grant_itr->agent_name == agent_name) {
            auto to_recall = safe_pct(pct, grant_itr->share);
            amount = recall_proxied_traversal(purpose_code, token_code, agents_idx, grants_idx, grant_itr->agent_name, to_recall, grant_itr->break_fee);
            if (grant_itr->pct || grant_itr->share > to_recall) {
                _db.modify(*grant_itr, [&](auto& g) { g.share -= to_recall; });
                ++grant_itr;
            }
            else {
                const auto &cur_grant = *grant_itr;
                ++grant_itr;
                _db.remove(cur_grant);
            }
            break;
        }
        else
            ++grant_itr;
    }
    
    EOS_ASSERT(amount > 0, transaction_exception, "amount to recall must be positive");
    _db.modify(*grantor_as_agent, [&](auto& a) {
        a.balance += amount;
        a.proxied -= amount;
    });
}

int64_t resource_limits_manager::recall_proxied_traversal(symbol_code purpose_code, symbol_code token_code, 
                    const agents_idx_t& agents_idx, const grants_idx_t& grants_idx, 
                    const account_name& agent_name, int64_t share, int16_t break_fee) const{
    
    auto agent = get_agent(purpose_code, token_code, agents_idx, agent_name);

    EOS_ASSERT(share >= 0, transaction_exception, "SYSTEM: share can't be negative");
    EOS_ASSERT(share <= agent->shares_sum, transaction_exception, "SYSTEM: incorrect share val");
    if(share == 0)
        return 0;
        
    //TODO: fee should be taken _from the profit_
    auto share_fee = safe_pct(std::min(agent->fee, break_fee), share);
    auto share_net = share - share_fee;
    auto balance_ret = safe_prop(agent->balance, share_net, agent->shares_sum);
    EOS_ASSERT(balance_ret <= agent->balance, transaction_exception, "SYSTEM: incorrect balance_ret val");
    
    auto proxied_ret = 0;
    auto grant_itr = grants_idx.lower_bound(grant_key(purpose_code, token_code, agent->account));
    while ((grant_itr != grants_idx.end()) &&
           (grant_itr->purpose_code   == purpose_code) &&
           (grant_itr->token_code   == token_code) &&
           (grant_itr->grantor_name == agent->account))
    {
        auto to_recall = safe_prop(share_net, grant_itr->share, agent->shares_sum);
        proxied_ret += recall_proxied_traversal(purpose_code, token_code, agents_idx, grants_idx, grant_itr->agent_name, to_recall, grant_itr->break_fee);
        _db.modify(*grant_itr, [&](auto& g) { g.share -= to_recall; });
        ++grant_itr;
    }
    EOS_ASSERT(proxied_ret <= agent->proxied, transaction_exception, "SYSTEM: incorrect proxied_ret val");
    
    _db.modify(*agent, [&](auto& a) {
        a.balance -= balance_ret;
        a.proxied -= proxied_ret;
        a.own_share += share_fee;
        a.shares_sum -= share_net;
    });
    return balance_ret + proxied_ret;
}

void resource_limits_manager::update_proxied_traversal(int64_t now, symbol_code purpose_code, symbol_code token_code,
                    const agents_idx_t& agents_idx, const grants_idx_t& grants_idx,
                    const stake_agent_object* agent, int64_t frame_length, bool force) const{

    if ((now - agent->last_proxied_update.sec_since_epoch() >= frame_length) || force) {
        int64_t new_proxied = 0;
        int64_t unstaked = 0;

        auto grant_itr = grants_idx.lower_bound(grant_key(purpose_code, token_code, agent->account));
        
        while ((grant_itr != grants_idx.end()) &&
               (grant_itr->purpose_code == purpose_code) &&
               (grant_itr->token_code   == token_code) &&
               (grant_itr->grantor_name == agent->account))
        {
            auto proxy_agent = get_agent(purpose_code, token_code, agents_idx, grant_itr->agent_name);
            update_proxied_traversal(now, purpose_code, token_code, agents_idx, grants_idx, proxy_agent, frame_length, force);
            
            if (proxy_agent->proxy_level < agent->proxy_level && 
                grant_itr->break_fee >= proxy_agent->fee &&
                grant_itr->break_min_own_staked <= proxy_agent->min_own_staked) 
            {
                if (proxy_agent->shares_sum)
                    new_proxied += safe_prop(proxy_agent->get_total_funds(), grant_itr->share, proxy_agent->shares_sum);
                ++grant_itr;
            }
            else {
                unstaked += recall_proxied_traversal(purpose_code, token_code, agents_idx, grants_idx, grant_itr->agent_name, grant_itr->share, grant_itr->break_fee);
                const auto &cur_grant = *grant_itr;
                ++grant_itr;
                _db.remove(cur_grant);
            }
        }
        _db.modify(*agent, [&](auto& a) {
            a.balance += unstaked;
            a.proxied = new_proxied;
            a.last_proxied_update = time_point_sec(now);
        });
    }
}

} } } /// eosio::chain::resource_limits
