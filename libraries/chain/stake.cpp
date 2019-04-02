#include <eosio/chain/stake.hpp>
#include <eosio/chain/int_arithmetic.hpp>
namespace eosio { namespace chain { namespace stake {
using namespace int_arithmetic;

template<typename AgentIndex, typename GrantIndex, typename GrantItr>
int64_t recall_proxied_traversal(const cyberway::chaindb::ram_payer_info& ram, symbol_code token_code,
    const AgentIndex& agents_idx, const GrantIndex& grants_idx,
    GrantItr& arg_grant_itr, int64_t pct, bool forced_erase = false
) {

    auto share = safe_pct(arg_grant_itr->share, pct);
    auto agent = get_agent(token_code, agents_idx, arg_grant_itr->agent_name);

    EOS_ASSERT(share >= 0, transaction_exception, "SYSTEM: share can't be negative");
    EOS_ASSERT(share <= agent->shares_sum, transaction_exception, "SYSTEM: incorrect share val");
    
    int64_t ret = 0;
    auto grantor_funds = safe_prop(agent->get_total_funds(), arg_grant_itr->share, agent->shares_sum);
    
    if (share > 0 && grantor_funds > 0) {
        auto profit = std::max(grantor_funds - arg_grant_itr->granted, int64_t(0));
        auto agent_fee_pct = safe_prop<int64_t>(std::min(agent->fee, arg_grant_itr->break_fee), profit, grantor_funds);
        
        auto share_fee = safe_pct(agent_fee_pct, share);
        auto share_net = share - share_fee;
        auto balance_ret = safe_prop(agent->balance, share_net, agent->shares_sum);
        EOS_ASSERT(balance_ret <= agent->balance, transaction_exception, "SYSTEM: incorrect balance_ret val");

        auto proxied_ret = 0;
        auto grant_itr = grants_idx.lower_bound(grant_key(token_code, agent->account));
        while ((grant_itr != grants_idx.end()) &&
               (grant_itr->token_code   == token_code) &&
               (grant_itr->grantor_name == agent->account))
        {
            auto to_recall = safe_prop(share_net, grant_itr->share, agent->shares_sum);
            auto cur_pct = safe_prop<int64_t>(config::percent_100, share_net, agent->shares_sum);
            proxied_ret += recall_proxied_traversal(ram, token_code, agents_idx, grants_idx, grant_itr, cur_pct);
        }
        EOS_ASSERT(proxied_ret <= agent->proxied, transaction_exception, "SYSTEM: incorrect proxied_ret val");

        agents_idx.modify(*agent, [&](auto& a) {
            a.set_balance(a.balance - balance_ret);
            a.proxied -= proxied_ret;
            a.own_share += share_fee;
            a.shares_sum -= share_net;
        });
        
        ret = balance_ret + proxied_ret;
    }

    if ((arg_grant_itr->pct || arg_grant_itr->share > share) && !forced_erase) {
        if (share || pct) {
            grants_idx.modify(*arg_grant_itr, [&](auto& g) {
                g.share -= share;
                g.granted = safe_pct(config::percent_100 - pct, g.granted);
            });
        }
        ++arg_grant_itr;
    }
    else {
        const auto &arg_grant = *arg_grant_itr;
        ++arg_grant_itr;
        grants_idx.erase(arg_grant, ram);
    }
    
    return ret;
}

template<typename AgentIndex, typename GrantIndex>
void update_proxied_traversal(
    const cyberway::chaindb::ram_payer_info& ram, int64_t now, symbol_code token_code,
    const AgentIndex& agents_idx, const GrantIndex& grants_idx,
    const stake_agent_object* agent, int64_t frame_length, bool force
) {
    if ((now - agent->last_proxied_update.sec_since_epoch() >= frame_length) || force) {
        int64_t new_proxied = 0;
        int64_t unstaked = 0;

        auto grant_itr = grants_idx.lower_bound(grant_key(token_code, agent->account));

        while ((grant_itr != grants_idx.end()) && (grant_itr->token_code   == token_code) && (grant_itr->grantor_name == agent->account)) {
            auto proxy_agent = get_agent(token_code, agents_idx, grant_itr->agent_name);
            update_proxied_traversal(ram, now, token_code, agents_idx, grants_idx, proxy_agent, frame_length, force);

            if (proxy_agent->proxy_level < agent->proxy_level &&
                grant_itr->break_fee >= proxy_agent->fee &&
                grant_itr->break_min_own_staked <= proxy_agent->min_own_staked)
            {
                if (proxy_agent->shares_sum)
                    new_proxied += safe_prop(proxy_agent->get_total_funds(), grant_itr->share, proxy_agent->shares_sum);
                ++grant_itr;
            }
            else {
                unstaked += recall_proxied_traversal(ram, token_code, agents_idx, grants_idx, grant_itr, config::percent_100, true);
            }
        }
        agents_idx.modify(*agent, [&](auto& a) {
            a.set_balance(a.balance + unstaked);
            a.proxied = new_proxied;
            a.last_proxied_update = time_point_sec(now);
        });
    }
}

void update_proxied(cyberway::chaindb::chaindb_controller& db, const cyberway::chaindb::ram_payer_info& ram, int64_t now, 
                    symbol_code token_code, const account_name& account, int64_t frame_length, bool force) {

    auto agents_table = db.get_table<stake_agent_object>();
    auto grants_table = db.get_table<stake_grant_object>();
    auto agents_idx = agents_table.get_index<stake_agent_object::by_key>();
    auto grants_idx = grants_table.get_index<stake_grant_object::by_key>();
    update_proxied_traversal(ram, now, token_code, agents_idx, grants_idx,
        get_agent(token_code, agents_idx, account),
        frame_length, force);
}

void recall_proxied(cyberway::chaindb::chaindb_controller& db, const cyberway::chaindb::ram_payer_info& ram, int64_t now, 
                    symbol_code token_code, account_name grantor_name, account_name agent_name, int16_t pct) {
                        
    EOS_ASSERT(1 <= pct && pct <= config::percent_100, transaction_exception, "pct must be between 0.01% and 100% (1-10000)");
    const auto* param = db.find<stake_param_object, by_id>(token_code.value);
    EOS_ASSERT(param, transaction_exception, "no staking for token");

    auto agents_table = db.get_table<stake_agent_object>();
    auto grants_table = db.get_table<stake_grant_object>();
    auto agents_idx = agents_table.get_index<stake_agent_object::by_key>();
    auto grants_idx = grants_table.get_index<stake_grant_object::by_key>();

    auto grantor_as_agent = get_agent(token_code, agents_idx, grantor_name);
    
    update_proxied_traversal(ram, now, token_code, agents_idx, grants_idx, grantor_as_agent, param->frame_length, false);
    
    int64_t amount = 0;
    auto grant_itr = grants_idx.lower_bound(grant_key(token_code, grantor_name));
    while ((grant_itr != grants_idx.end()) && (grant_itr->token_code   == token_code) && (grant_itr->grantor_name == grantor_name)) {
        if (grant_itr->agent_name == agent_name) {
            amount = recall_proxied_traversal(ram, token_code, agents_idx, grants_idx, grant_itr, pct);
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

