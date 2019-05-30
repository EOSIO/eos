#pragma once
#include <eosio/chain/stake_object.hpp>
#include <cyberway/chaindb/common.hpp>
namespace eosio { namespace chain { namespace stake {
using stake_index_set = index_set<
   stake_agent_table,
   stake_grant_table,
   stake_param_table,
   stake_stat_table
>;

inline auto agent_key(symbol_code token_code, const account_name& agent_name) {
    return boost::make_tuple(token_code, agent_name);
}
inline auto grant_key(symbol_code token_code, const account_name& grantor_name,
    const account_name& agent_name = account_name()
) {
    return boost::make_tuple(token_code, grantor_name, agent_name);
}

template<typename AgentIndex>
const stake_agent_object* get_agent(symbol_code token_code, const AgentIndex& agents_idx, const account_name& agent_name) {
    auto agent = agents_idx.find(agent_key(token_code, agent_name));
    EOS_ASSERT(agent != agents_idx.end(), transaction_exception, "agent doesn't exist");
    return &(*agent);
}

void update_proxied(cyberway::chaindb::chaindb_controller& db, const cyberway::chaindb::storage_payer_info&, int64_t now,
                    symbol_code token_code, const account_name& account, bool force);
void recall_proxied(cyberway::chaindb::chaindb_controller& db, const cyberway::chaindb::storage_payer_info&, int64_t now,
                    symbol_code token_code, account_name grantor_name, account_name agent_name, int16_t pct);
} } }/// eosio::chain::stake
