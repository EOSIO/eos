#pragma once

#include "ee_genesis_serializer.hpp"
#include "golos_operations.hpp"
#include <fc/crypto/sha256.hpp>

namespace cyberway { namespace genesis { namespace ee {

using namespace eosio::chain;

class event_engine_genesis final {
public:
    event_engine_genesis(const event_engine_genesis&) = delete;
    event_engine_genesis() = default;

    void start(const bfs::path& ee_directory, const fc::sha256& hash);
    void finalize();

    enum ee_ser_type {messages, transfers, delegations, rewards, pinblocks, accounts, witnesses, funds, balance_conversions};
    ee_genesis_serializer& get_serializer(ee_ser_type type) {
        return serializers.at(type);
    }

private:
    std::map<ee_ser_type, ee_genesis_serializer> serializers;

};

struct vote_info {
    OBJECT_CTOR(vote_info);

    name voter;
    int16_t weight;
    fc::time_point_sec time;
    int64_t rshares;
};

struct reblog_info {
    OBJECT_CTOR(reblog_info);

    name account;
    string title;
    string body;
    fc::time_point_sec time;
};

struct comment_info {
    OBJECT_CTOR(comment_info);

    name parent_author;
    string parent_permlink;
    name author;
    string permlink;
    fc::time_point_sec created;
    fc::time_point_sec last_update;
    string title;
    string body;
    fc::flat_set<string> tags;
    string language;
    int64_t net_rshares;
    uint16_t rewardweight = 0;
    asset max_payout;
    uint16_t benefics_prcnt = 0;
    uint16_t curators_prcnt = 0;
    uint16_t tokenprop = 0;
    bool archived = true;
    asset author_reward;
    asset benefactor_reward;
    asset curator_reward;
    std::vector<vote_info> votes;
    std::vector<reblog_info> reblogs;
};

struct transfer_info {
    OBJECT_CTOR(transfer_info);

    name from;
    name to;
    asset quantity;
    string memo;
    bool to_vesting;
    fc::time_point_sec time;
};

struct author_reward {
    OBJECT_CTOR(author_reward);

    name author;
    string permlink;
    asset sbd_and_steem_payout;
    asset vesting_payout;
    fc::time_point_sec time;
};

struct curation_reward {
    OBJECT_CTOR(curation_reward);

    name curator;
    asset reward;
    name comment_author;
    string comment_permlink;
    fc::time_point_sec time;
};

struct delegation_reward {
    OBJECT_CTOR(delegation_reward);

    name delegator;
    name delegatee;
    cyberway::golos::ee::delegator_payout_strategy payout_strategy;
    asset reward;
    fc::time_point_sec time;
};

struct balance_convert_info {
    OBJECT_CTOR(balance_convert_info);

    name owner;
    asset amount;
    string memo;
};

struct pin_info {
    OBJECT_CTOR(pin_info);

    name pinner;
    name pinning;
};

struct block_info {
    OBJECT_CTOR(block_info);

    name blocker;
    name blocking;
};

} } } // cyberway::genesis::ee

FC_REFLECT(cyberway::genesis::ee::vote_info, (voter)(weight)(time)(rshares))
FC_REFLECT(cyberway::genesis::ee::reblog_info, (account)(title)(body)(time))
FC_REFLECT(cyberway::genesis::ee::comment_info, (parent_author)(parent_permlink)(author)(permlink)(created)(last_update)
    (title)(body)(tags)(language)(net_rshares)(rewardweight)(max_payout)(benefics_prcnt)(curators_prcnt)(tokenprop)(archived)
    (author_reward)(benefactor_reward)(curator_reward)(votes)(reblogs))
FC_REFLECT(cyberway::genesis::ee::transfer_info, (from)(to)(quantity)(memo)(to_vesting)(time))
FC_REFLECT(cyberway::genesis::ee::author_reward, (author)(permlink)(sbd_and_steem_payout)(vesting_payout)(time))
FC_REFLECT(cyberway::genesis::ee::curation_reward, (curator)(reward)(comment_author)(comment_permlink)(time))
FC_REFLECT(cyberway::genesis::ee::delegation_reward, (delegator)(delegatee)(payout_strategy)(reward)(time))
FC_REFLECT(cyberway::genesis::ee::balance_convert_info, (owner)(amount)(memo))
FC_REFLECT(cyberway::genesis::ee::pin_info, (pinner)(pinning))
FC_REFLECT(cyberway::genesis::ee::block_info, (blocker)(blocking))
