/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/symbol.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/database_utils.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/multi_index_includes.hpp>

namespace eosio { namespace chain {

class stake_agent_object : public cyberway::chaindb::object<stake_agent_object_type, stake_agent_object> {
    OBJECT_CTOR(stake_agent_object)
    id_type id;  
    symbol_code token_code;
    account_name account;
    uint8_t proxy_level;
    int64_t votes;
    time_point_sec last_proxied_update;
    int64_t balance;
    int64_t proxied;
    int64_t shares_sum;
    int64_t own_share;
    int16_t fee;
    int64_t min_own_staked;
    public_key_type signing_key;
    int64_t get_total_funds()const { return balance + proxied; };
    struct by_key {};
    struct by_votes {};
    void set_balance(int64_t arg) {
        balance = arg;
        if (!proxy_level) {
            votes = balance;
        }
    };
};

using stake_agent_table = cyberway::chaindb::table_container<
    stake_agent_object,
    cyberway::chaindb::indexed_by<
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, 
           BOOST_MULTI_INDEX_MEMBER(stake_agent_object, stake_agent_object::id_type, id)>,
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<stake_agent_object::by_key>,
           cyberway::chaindb::composite_key<stake_agent_object,
              BOOST_MULTI_INDEX_MEMBER(stake_agent_object, symbol_code, token_code),
              BOOST_MULTI_INDEX_MEMBER(stake_agent_object, account_name, account)>
        >,
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<stake_agent_object::by_votes>,
           cyberway::chaindb::composite_key<stake_agent_object,
              BOOST_MULTI_INDEX_MEMBER(stake_agent_object, symbol_code, token_code),
              BOOST_MULTI_INDEX_MEMBER(stake_agent_object, int64_t, votes),
              BOOST_MULTI_INDEX_MEMBER(stake_agent_object, account_name, account)>
        >
    >
>;

class stake_grant_object : public cyberway::chaindb::object<stake_grant_object_type, stake_grant_object> {
    OBJECT_CTOR(stake_grant_object)
    id_type id;
    symbol_code token_code;
    account_name grantor_name;
    account_name agent_name;
    int16_t pct;
    int64_t share;
    int16_t break_fee;
    int64_t break_min_own_staked;
    
    struct by_key {};
};

using stake_grant_table = cyberway::chaindb::table_container<
    stake_grant_object,
    cyberway::chaindb::indexed_by<
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>,
            BOOST_MULTI_INDEX_MEMBER(stake_grant_object, stake_grant_object::id_type, id)>,
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<stake_grant_object::by_key>,
           cyberway::chaindb::composite_key<stake_grant_object,
              BOOST_MULTI_INDEX_MEMBER(stake_grant_object, symbol_code, token_code),
              BOOST_MULTI_INDEX_MEMBER(stake_grant_object, account_name, grantor_name),
              BOOST_MULTI_INDEX_MEMBER(stake_grant_object, account_name, agent_name)>
        >
    >
>;

    
class stake_param_object : public cyberway::chaindb::object<stake_param_object_type, stake_param_object> {
    OBJECT_CTOR(stake_param_object)
    id_type id;
    symbol token_symbol;
    std::vector<uint8_t> max_proxies;
    int64_t payout_step_length;
    uint16_t payout_steps_num;
    int64_t min_own_staked_for_election = 0;
};

using stake_param_table = cyberway::chaindb::table_container<
    stake_param_object,
    cyberway::chaindb::indexed_by<
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(stake_param_object, stake_param_object::id_type, id)>
    >
>;
    
class stake_stat_object : public cyberway::chaindb::object<stake_stat_object_type, stake_stat_object> {
    OBJECT_CTOR(stake_stat_object)
    id_type id;
    symbol_code token_code;
    int64_t total_staked;
    time_point_sec last_reward;
    bool enabled;
};

using stake_stat_table = cyberway::chaindb::table_container<
    stake_stat_object,
    cyberway::chaindb::indexed_by<
        cyberway::chaindb::ordered_unique<cyberway::chaindb::tag<by_id>, BOOST_MULTI_INDEX_MEMBER(stake_stat_object, stake_stat_object::id_type, id)>
    >
>;

} } // eosio::chain

CHAINDB_SET_TABLE_TYPE(eosio::chain::stake_agent_object, eosio::chain::stake_agent_table)
CHAINDB_TAG(eosio::chain::stake_agent_object::by_key, bykey)
CHAINDB_TAG(eosio::chain::stake_agent_object::by_votes, byvotes)
CHAINDB_TAG(eosio::chain::stake_agent_object, stake.agent)

CHAINDB_SET_TABLE_TYPE(eosio::chain::stake_grant_object, eosio::chain::stake_grant_table)
CHAINDB_TAG(eosio::chain::stake_grant_object::by_key, bykey)
CHAINDB_TAG(eosio::chain::stake_grant_object, stake.grant)

CHAINDB_SET_TABLE_TYPE(eosio::chain::stake_param_object, eosio::chain::stake_param_table)
CHAINDB_TAG(eosio::chain::stake_param_object, stake.param)

CHAINDB_SET_TABLE_TYPE(eosio::chain::stake_stat_object, eosio::chain::stake_stat_table)
CHAINDB_TAG(eosio::chain::stake_stat_object, stake.stat)

FC_REFLECT(eosio::chain::stake_agent_object, 
    (id)(token_code)(account)(proxy_level)(votes)(last_proxied_update)(balance)
    (proxied)(shares_sum)(own_share)(fee)(min_own_staked)(signing_key))
    
FC_REFLECT(eosio::chain::stake_grant_object, 
    (id)(token_code)(grantor_name)(agent_name)(pct)(share)(break_fee)(break_min_own_staked))
    
FC_REFLECT(eosio::chain::stake_param_object, 
    (id)(token_symbol)(max_proxies)(payout_step_length)(payout_steps_num)(min_own_staked_for_election))
    
FC_REFLECT(eosio::chain::stake_stat_object, 
    (id)(token_code)(total_staked)(last_reward)(enabled))
