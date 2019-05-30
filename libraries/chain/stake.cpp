#include <eosio/chain/stake.hpp>
#include <eosio/chain/int_arithmetic.hpp>
namespace eosio { namespace chain { namespace stake {
using namespace int_arithmetic;

template<typename AgentIndex, typename GrantIndex, typename GrantItr>
int64_t recall_proxied_traversal(const cyberway::chaindb::storage_payer_info& storage, symbol_code token_code,
    const AgentIndex& agents_idx, const GrantIndex& grants_idx,
    GrantItr& arg_grant_itr, int64_t share, bool forced_erase = false
) {
    auto agent = get_agent(token_code, agents_idx, arg_grant_itr->agent_name);

    EOS_ASSERT(share >= 0, transaction_exception, "SYSTEM: share can't be negative");
    EOS_ASSERT(share <= agent->shares_sum, transaction_exception, "SYSTEM: incorrect share val");
    
    int64_t ret = 0;
    
    auto balance_ret = safe_prop(agent->balance, share, agent->shares_sum);
    EOS_ASSERT(balance_ret <= agent->balance, transaction_exception, "SYSTEM: incorrect balance_ret val");

    auto proxied_ret = 0;
    auto grant_itr = grants_idx.lower_bound(grant_key(token_code, agent->account));
    while ((grant_itr != grants_idx.end()) && (grant_itr->token_code == token_code) && (grant_itr->grantor_name == agent->account)) {
        auto cur_share = safe_prop(grant_itr->share, share, agent->shares_sum);
        proxied_ret += recall_proxied_traversal(storage, token_code, agents_idx, grants_idx, grant_itr, cur_share);
    }
    EOS_ASSERT(proxied_ret <= agent->proxied, transaction_exception, "SYSTEM: incorrect proxied_ret val");

    agents_idx.modify(*agent, [&](auto& a) {
        a.set_balance(a.balance - balance_ret);
        a.proxied -= proxied_ret;
        a.shares_sum -= share;
    });
    
    ret = balance_ret + proxied_ret;

    if ((arg_grant_itr->pct || arg_grant_itr->share > share) && !forced_erase) {
        grants_idx.modify(*arg_grant_itr, [&](auto& g) { g.share -= share; });
        ++arg_grant_itr;
    }
    else {
        const auto &arg_grant = *arg_grant_itr;
        ++arg_grant_itr;
        grants_idx.erase(arg_grant, storage);
    }
    
    return ret;
}

template<typename AgentIndex, typename GrantIndex>
void update_proxied_traversal(
    const cyberway::chaindb::storage_payer_info& ram, int64_t now, symbol_code token_code,
    const AgentIndex& agents_idx, const GrantIndex& grants_idx,
    const stake_agent_object* agent, time_point_sec last_reward, bool force
) {
    if ((last_reward >= agent->last_proxied_update) || force) {
        int64_t new_proxied = 0;
        int64_t recalled = 0;

        auto grant_itr = grants_idx.lower_bound(grant_key(token_code, agent->account));

        while ((grant_itr != grants_idx.end()) && (grant_itr->token_code   == token_code) && (grant_itr->grantor_name == agent->account)) {
            auto proxy_agent = get_agent(token_code, agents_idx, grant_itr->agent_name);
            update_proxied_traversal(ram, now, token_code, agents_idx, grants_idx, proxy_agent, last_reward, force);

            if (proxy_agent->proxy_level < agent->proxy_level &&
                grant_itr->break_fee >= proxy_agent->fee &&
                grant_itr->break_min_own_staked <= proxy_agent->min_own_staked)
            {
                if (proxy_agent->shares_sum)
                    new_proxied += safe_prop(proxy_agent->get_total_funds(), grant_itr->share, proxy_agent->shares_sum);
                ++grant_itr;
            }
            else {
                recalled += recall_proxied_traversal(ram, token_code, agents_idx, grants_idx, grant_itr, grant_itr->share, true);
            }
        }
        agents_idx.modify(*agent, [&](auto& a) {
            a.set_balance(a.balance + recalled);
            a.proxied = new_proxied;
            a.last_proxied_update = time_point_sec(now);
        });
    }
}

void update_proxied(cyberway::chaindb::chaindb_controller& db, const cyberway::chaindb::storage_payer_info& storage, int64_t now,
                    symbol_code token_code, const account_name& account, bool force) {
    auto stat = db.find<stake_stat_object, by_id>(token_code.value);
    EOS_ASSERT(stat, transaction_exception, "no staking for token");
    auto agents_table = db.get_table<stake_agent_object>();
    auto grants_table = db.get_table<stake_grant_object>();
    auto agents_idx = agents_table.get_index<stake_agent_object::by_key>();
    auto grants_idx = grants_table.get_index<stake_grant_object::by_key>();
    update_proxied_traversal(storage, now, token_code, agents_idx, grants_idx,
        get_agent(token_code, agents_idx, account),
        stat->last_reward, force);
}

void recall_proxied(cyberway::chaindb::chaindb_controller& db, const cyberway::chaindb::storage_payer_info& storage, int64_t now,
                    symbol_code token_code, account_name grantor_name, account_name agent_name, int16_t pct) {
                        
    EOS_ASSERT(1 <= pct && pct <= config::percent_100, transaction_exception, "pct must be between 0.01% and 100% (1-10000)");
    EOS_ASSERT((db.find<stake_param_object, by_id>(token_code.value)), transaction_exception, "no staking for token");

    auto agents_table = db.get_table<stake_agent_object>();
    auto grants_table = db.get_table<stake_grant_object>();
    auto agents_idx = agents_table.get_index<stake_agent_object::by_key>();
    auto grants_idx = grants_table.get_index<stake_grant_object::by_key>();

    auto grantor_as_agent = get_agent(token_code, agents_idx, grantor_name);
    
    update_proxied_traversal(storage, now, token_code, agents_idx, grants_idx, grantor_as_agent, time_point_sec(), true);
    
    int64_t amount = 0;
    auto grant_itr = grants_idx.lower_bound(grant_key(token_code, grantor_name));
    while ((grant_itr != grants_idx.end()) && (grant_itr->token_code == token_code) && (grant_itr->grantor_name == grantor_name)) {
        if (grant_itr->agent_name == agent_name) {
            amount = recall_proxied_traversal(storage, token_code, agents_idx, grants_idx, grant_itr, safe_pct<int64_t>(pct, grant_itr->share));
            break;
        }
        else
            ++grant_itr;
    }
    
    EOS_ASSERT(amount > 0, transaction_exception, "amount to recall must be positive");
    agents_table.modify(*grantor_as_agent, [&](auto& a) {
        a.set_balance(a.balance + amount);
        a.proxied -= amount;
    });
}

} } }/// eosio::chain::stake

