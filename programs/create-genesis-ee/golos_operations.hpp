#pragma once

#include "golos_types.hpp"

namespace cyberway { namespace golos {

struct comment_operation : hashed_operation {
    account_name_type parent_author;
    string parent_permlink;

    account_name_type author;
    string permlink;

    string title;
    string body;
    flat_set<string> tags;
    string language;
    fc::time_point_sec timestamp;
};

struct delete_comment_operation : hashed_operation {
};

struct vote_operation : hashed_operation {
    account_name_type voter;
    account_name_type author;
    string permlink;
    int16_t weight = 0;
    int64_t rshares;
    fc::time_point_sec timestamp;
};

struct reblog_operation : hashed_operation {
    account_name_type account;
    account_name_type author;
    string permlink;
    string title;
    string body;
    fc::time_point_sec timestamp;
};

struct delete_reblog_operation : hashed_operation {
    account_name_type account;
};

struct transfer_operation : operation {
    account_name_type from;
    /// Account to transfer asset to
    account_name_type to;
    /// The amount of asset to transfer from @ref from to @ref to
    asset amount;

    /// The memo is plain-text, any encryption on the memo is up to
    /// a higher level protocol.
    string memo;
    fc::time_point_sec timestamp;
};

enum follow_type {
    undefined,
    blog,
    ignore
};

struct follow_operation : hashed_operation {
    account_name_type follower;
    account_name_type following;
    uint16_t what;
};

struct author_reward_operation : hashed_operation {
    account_name_type author;
    string permlink;
    asset sbd_payout;
    asset steem_payout;
    asset vesting_payout;
};

struct comment_benefactor_reward_operation : hashed_operation {
    account_name_type benefactor;
    account_name_type author;
    string permlink;
    asset reward;
};

struct curation_reward_operation : hashed_operation {
    account_name_type curator;
    asset reward;
    account_name_type comment_author;
    string comment_permlink;
};

struct auction_window_reward_operation : hashed_operation {
    asset reward;
    account_name_type comment_author;
    string comment_permlink;
};

struct total_comment_reward_operation : hashed_operation {
    account_name_type author;
    string permlink;
    asset author_reward;
    asset benefactor_reward;
    asset curator_reward;
    int64_t net_rshares;
};

} } // cyberway::golos

REFLECT_OP_HASHED(cyberway::golos::comment_operation, (parent_author)(parent_permlink)(author)(permlink)(title)(body)(tags)(language)(timestamp))

REFLECT_OP_HASHED(cyberway::golos::delete_comment_operation, )

REFLECT_OP_HASHED(cyberway::golos::vote_operation, (voter)(author)(permlink)(weight)(rshares)(timestamp))

REFLECT_OP_HASHED(cyberway::golos::reblog_operation, (account)(author)(permlink)(title)(body)(timestamp))

REFLECT_OP_HASHED(cyberway::golos::delete_reblog_operation, (account))

REFLECT_OP(cyberway::golos::transfer_operation, (from)(to)(amount)(memo)(timestamp))

REFLECT_OP_HASHED(cyberway::golos::follow_operation, (follower)(following)(what))

REFLECT_OP_HASHED(cyberway::golos::author_reward_operation, (author)(permlink)(sbd_payout)(steem_payout)(vesting_payout))

REFLECT_OP_HASHED(cyberway::golos::curation_reward_operation, (curator)(reward)(comment_author)(comment_permlink))

REFLECT_OP_HASHED(cyberway::golos::auction_window_reward_operation, (reward)(comment_author)(comment_permlink))

REFLECT_OP_HASHED(cyberway::golos::comment_benefactor_reward_operation, (benefactor)(author)(permlink)(reward))

REFLECT_OP_HASHED(cyberway::golos::total_comment_reward_operation, (author)(permlink)(author_reward)(benefactor_reward)(curator_reward)(net_rshares))
