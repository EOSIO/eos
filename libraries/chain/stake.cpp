#include <eosio/chain/stake.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/int_arithmetic.hpp>
#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/names.hpp>

namespace eosio { namespace chain { namespace stake {
using namespace int_arithmetic;

void set_votes(cyberway::chaindb::chaindb_controller& db, const stake_stat_object* stat, symbol_code token_code, const std::map<account_name, int64_t>& votes_changes) {
    int64_t votes_changes_sum = 0;
    for (const auto& v : votes_changes) {
        votes_changes_sum += v.second;
    }
    db.get_table<stake_stat_object>().modify(*stat, [&](auto& s) { s.total_votes += votes_changes_sum; });
    
    auto stat_itr = db.begin({config::token_account_name, token_code, N(stat), cyberway::chaindb::names::primary_index});
    EOS_ASSERT(stat_itr.pk != cyberway::chaindb::primary_key::End, transaction_exception, "SYSTEM: token doesn't exist");
    const auto stat_obj = db.value_at_cursor({config::token_account_name, stat_itr.cursor});
    asset supply;
    fc::from_variant(stat_obj["supply"], supply);
    
    auto candidates_table = db.get_table<stake_candidate_object>();
    auto candidates_idx = candidates_table.get_index<stake_candidate_object::by_key>();
    for (const auto& v : votes_changes) {
        auto cand = candidates_idx.find(agent_key(token_code, v.first));
        EOS_ASSERT(cand != candidates_idx.end(), transaction_exception, "SYSTEM: candidate doesn't exist");
        candidates_idx.modify(*cand, [&](auto& a) { a.set_votes(a.votes + v.second, supply.get_amount()); });
    }
}

template<typename AgentIndex, typename GrantIndex, typename GrantItr>
int64_t recall_proxied_traversal(const cyberway::chaindb::storage_payer_info& storage, symbol_code token_code,
    const AgentIndex& agents_idx, const GrantIndex& grants_idx,
    GrantItr& arg_grant_itr, int64_t share, std::map<account_name, int64_t>& votes_changes, bool forced_erase = false
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
        proxied_ret += recall_proxied_traversal(storage, token_code, agents_idx, grants_idx, grant_itr, cur_share, votes_changes);
    }
    EOS_ASSERT(proxied_ret <= agent->proxied, transaction_exception, "SYSTEM: incorrect proxied_ret val");

    agents_idx.modify(*agent, [&](auto& a) {
        a.balance -= balance_ret;
        a.proxied -= proxied_ret;
        a.shares_sum -= share;
    });
    
    if (!agent->proxy_level) {
        votes_changes[agent->account] -= balance_ret;
    }
    
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
    const stake_agent_object* agent, time_point_sec last_reward, std::map<account_name, int64_t>& votes_changes, bool force
) {
    if ((last_reward >= agent->last_proxied_update) || force) {
        int64_t new_proxied = 0;
        int64_t recalled = 0;

        auto grant_itr = grants_idx.lower_bound(grant_key(token_code, agent->account));

        while ((grant_itr != grants_idx.end()) && (grant_itr->token_code == token_code) && (grant_itr->grantor_name == agent->account)) {
            auto proxy_agent = get_agent(token_code, agents_idx, grant_itr->agent_name);
            update_proxied_traversal(ram, now, token_code, agents_idx, grants_idx, proxy_agent, last_reward, votes_changes, force);

            if (proxy_agent->proxy_level < agent->proxy_level &&
                grant_itr->break_fee >= proxy_agent->fee &&
                grant_itr->break_min_own_staked <= proxy_agent->min_own_staked)
            {
                if (proxy_agent->shares_sum)
                    new_proxied += safe_prop(proxy_agent->get_total_funds(), grant_itr->share, proxy_agent->shares_sum);
                ++grant_itr;
            }
            else {
                recalled += recall_proxied_traversal(ram, token_code, agents_idx, grants_idx, grant_itr, grant_itr->share, votes_changes, true);
            }
        }
        agents_idx.modify(*agent, [&](auto& a) {
            a.balance += recalled; //this agent can't be a candidate
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
    std::map<account_name, int64_t> votes_changes;
    update_proxied_traversal(storage, now, token_code, agents_idx, grants_idx,
        get_agent(token_code, agents_idx, account),
        stat->last_reward, votes_changes, force);
    set_votes(db, &(*stat), token_code, votes_changes);
}

void recall_proxied(cyberway::chaindb::chaindb_controller& db, const cyberway::chaindb::storage_payer_info& storage, int64_t now,
                    symbol_code token_code, account_name grantor_name, account_name agent_name, int16_t pct) {
                        
    EOS_ASSERT(1 <= pct && pct <= config::percent_100, transaction_exception, "pct must be between 0.01% and 100% (1-10000)");
    EOS_ASSERT((db.find<stake_param_object, by_id>(token_code.value)), transaction_exception, "no staking for token");
    
    auto stat = db.find<stake_stat_object, by_id>(token_code.value);
    EOS_ASSERT(stat, transaction_exception, "no staking for token");

    auto agents_table = db.get_table<stake_agent_object>();
    auto grants_table = db.get_table<stake_grant_object>();
    auto agents_idx = agents_table.get_index<stake_agent_object::by_key>();
    auto grants_idx = grants_table.get_index<stake_grant_object::by_key>();

    auto grantor_as_agent = get_agent(token_code, agents_idx, grantor_name);
    std::map<account_name, int64_t> votes_changes;
    update_proxied_traversal(storage, now, token_code, agents_idx, grants_idx, grantor_as_agent, time_point_sec(), votes_changes, true);
    
    int64_t amount = 0;
    auto grant_itr = grants_idx.lower_bound(grant_key(token_code, grantor_name));
    while ((grant_itr != grants_idx.end()) && (grant_itr->token_code == token_code) && (grant_itr->grantor_name == grantor_name)) {
        if (grant_itr->agent_name == agent_name) {
            amount = recall_proxied_traversal(storage, token_code, agents_idx, grants_idx, grant_itr, safe_pct<int64_t>(pct, grant_itr->share), votes_changes);
            break;
        }
        else
            ++grant_itr;
    }
    
    EOS_ASSERT(amount > 0, transaction_exception, "amount to recall must be positive");
    agents_table.modify(*grantor_as_agent, [&](auto& a) {
        a.balance += amount; //this agent can't be a candidate
        a.proxied -= amount;
    });
    set_votes(db, &(*stat), token_code, votes_changes);
}

} } }/// eosio::chain::stake

