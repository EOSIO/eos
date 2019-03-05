#pragma once
#include "golos_types.hpp"

namespace cw { namespace golos {


using fc::uint128_t;
using fc::time_point_sec;

using witness_id_type = id_type;
using account_id_type = id_type;
using comment_id_type = id_type;


struct dynamic_global_property_object {
    id_type id;
    uint32_t head_block_number;
    block_id_type head_block_id;
    time_point_sec time;
    account_name_type current_witness;
    uint64_t total_pow;
    uint32_t num_pow_witnesses;
    asset virtual_supply;
    asset current_supply;
    asset confidential_supply;
    asset current_sbd_supply;
    asset confidential_sbd_supply;
    asset total_vesting_fund_steem;
    asset total_vesting_shares;
    asset total_reward_fund_steem;
    uint128_t total_reward_shares2;
    uint16_t sbd_interest_rate;
    uint16_t sbd_print_rate;
    bool is_forced_min_price;
    uint32_t average_block_size;
    uint32_t maximum_block_size;
    uint64_t current_aslot;
    uint128_t recent_slots_filled;
    uint8_t participation_count;
    uint32_t last_irreversible_block_num;
    uint64_t max_virtual_bandwidth;
    uint64_t current_reserve_ratio;
    uint32_t vote_regeneration_per_day;
    uint16_t custom_ops_bandwidth_multiplier;
};


// account
struct account_object {
    id_type id;
    account_name_type name;
    public_key_type memo_key;
    account_name_type proxy;
    time_point_sec last_account_update;
    time_point_sec created;
    bool mined;
    bool owner_challenged;
    bool active_challenged;
    time_point_sec last_owner_proved;
    time_point_sec last_active_proved;
    account_name_type recovery_account;
    account_name_type reset_account;
    time_point_sec last_account_recovery;
    uint32_t comment_count;
    uint32_t lifetime_vote_count;
    uint32_t post_count;
    bool can_vote;
    uint16_t voting_power;
    uint16_t posts_capacity;
    uint16_t comments_capacity;
    uint16_t voting_capacity;
    time_point_sec last_vote_time;
    asset balance;
    asset savings_balance;
    asset sbd_balance;
    uint128_t sbd_seconds;
    time_point_sec sbd_seconds_last_update;
    time_point_sec sbd_last_interest_payment;
    asset savings_sbd_balance;
    uint128_t savings_sbd_seconds;
    time_point_sec savings_sbd_seconds_last_update;
    time_point_sec savings_sbd_last_interest_payment;
    uint8_t savings_withdraw_requests;
    share_type benefaction_rewards;
    share_type curation_rewards;
    share_type delegation_rewards;
    share_type posting_rewards;
    asset vesting_shares;
    asset delegated_vesting_shares;
    asset received_vesting_shares;
    asset vesting_withdraw_rate;
    time_point_sec next_vesting_withdrawal;
    share_type withdrawn;
    share_type to_withdraw;
    uint16_t withdraw_routes;
    fc::array<share_type, 4> proxied_vsf_votes;
    uint16_t witnesses_voted_for;
    time_point_sec last_post;
    account_name_type referrer_account;
    uint16_t referrer_interest_rate;
    time_point_sec referral_end_date;
    asset referral_break_fee;
};
struct account_authority_object {
    id_type id;
    account_name_type account;
    shared_authority owner;
    shared_authority active;
    shared_authority posting;
    time_point_sec last_owner_update;
};

enum bandwidth_type {post, forum, market, custom_json};
struct account_bandwidth_object {
    id_type id;
    account_name_type account;
    bandwidth_type type;
    share_type average_bandwidth;
    share_type lifetime_bandwidth;
    time_point_sec last_bandwidth_update;
};
struct account_metadata_object {
    id_type id;
    account_name_type account;
    shared_string json_metadata;
};

enum delegator_payout_strategy {to_delegator, to_delegated_vesting, _size};
struct vesting_delegation_object {
    id_type id;
    account_name_type delegator;
    account_name_type delegatee;
    asset vesting_shares;
    uint16_t interest_rate;
    delegator_payout_strategy payout_strategy;
    time_point_sec min_delegation_time;
};
struct vesting_delegation_expiration_object {
    id_type id;
    account_name_type delegator;
    asset vesting_shares;
    time_point_sec expiration;
};

struct owner_authority_history_object {
    id_type id;
    account_name_type account;
    shared_authority previous_owner_authority;
    time_point_sec last_valid_time;
};

struct account_recovery_request_object {
    id_type id;
    account_name_type account_to_recover;
    shared_authority new_owner_authority;
    time_point_sec expires;
};
struct change_recovery_account_request_object {
    id_type id;
    account_name_type account_to_recover;
    account_name_type recovery_account;
    time_point_sec effective_on;
};


// witness
enum witness_schedule_type {top19, timeshare, miner, none};
struct witness_object {
    id_type id;
    account_name_type owner;
    time_point_sec created;
    shared_string url;
    uint32_t total_missed;
    uint64_t last_aslot;
    uint64_t last_confirmed_block_num;
    uint64_t pow_worker;
    public_key_type signing_key;
    chain_properties props;
    price sbd_exchange_rate;
    time_point_sec last_sbd_exchange_update;
    share_type votes;
    witness_schedule_type schedule;
    uint128_t virtual_last_update;
    uint128_t virtual_position;
    uint128_t virtual_scheduled_time;
    digest_type last_work;
    version running_version;
    hardfork_version hardfork_version_vote;
    time_point_sec hardfork_time_vote;
};
struct witness_vote_object {
    id_type id;
    witness_id_type witness;
    account_id_type account;
};
struct witness_schedule_object {
    id_type id;
    uint128_t current_virtual_time;
    uint32_t next_shuffle_block_num;
    fc::array<account_name_type, 21/*STEEMIT_MAX_WITNESSES*/> current_shuffled_witnesses;
    uint8_t num_scheduled_witnesses;
    uint8_t top19_weight;
    uint8_t timeshare_weight;
    uint8_t miner_weight;
    uint32_t witness_pay_normalization_factor;
    chain_properties median_props;
    version majority_version;
};

//block_summary
struct block_summary_object {
    id_type id;
    block_id_type block_id;
};


// comment
struct beneficiary_route_type {
    account_name_type account;
    uint16_t weight;
};
enum comment_mode {not_set, first_payout, second_payout, archived};
enum auction_window_reward_destination_type {to_reward_fund, to_curators, to_author};
struct comment_object {
    id_type id;
    account_name_type parent_author;
    shared_string parent_permlink;
    account_name_type author;
    shared_string permlink;
    time_point_sec created;
    time_point_sec last_payout;
    uint16_t depth;
    uint32_t children;
    uint128_t children_rshares2;
    share_type net_rshares;
    share_type abs_rshares;
    share_type vote_rshares;
    share_type children_abs_rshares;
    time_point_sec cashout_time;
    time_point_sec max_cashout_time;
    uint16_t reward_weight;
    int32_t net_votes;
    uint32_t total_votes;
    id_type root_comment;
    comment_mode mode;
    curation_curve curation_reward_curve;
    auction_window_reward_destination_type auction_window_reward_destination;
    uint16_t auction_window_size;
    asset max_accepted_payout;
    uint16_t percent_steem_dollars;
    bool allow_replies;
    bool allow_votes;
    bool allow_curation_rewards;
    uint16_t curation_rewards_percent;
    std::vector<beneficiary_route_type> beneficiaries;
};
struct delegator_vote_interest_rate {
    account_name_type account;
    uint16_t interest_rate;
    delegator_payout_strategy payout_strategy;
};
struct comment_vote_object {
    id_type id;
    account_id_type voter;
    comment_id_type comment;
    int64_t orig_rshares;
    int64_t rshares;
    int16_t vote_percent;
    uint16_t auction_time;
    time_point_sec last_update;
    int8_t num_changes;
    std::vector<delegator_vote_interest_rate> delegator_vote_interest_rates;
};


// steem
struct limit_order_object {
    id_type id;
    time_point_sec created;
    time_point_sec expiration;
    account_name_type seller;
    uint32_t orderid;
    share_type for_sale;
    price sell_price;
};
struct convert_request_object {
    id_type id;
    account_name_type owner;
    uint32_t requestid;
    asset amount;
    time_point_sec conversion_date;
};
struct liquidity_reward_balance_object {
    id_type id;
    account_id_type owner;
    int64_t steem_volume;
    int64_t sbd_volume;
    uint128_t weight;
    time_point_sec last_update;
};
struct withdraw_vesting_route_object {
    id_type id;
    account_id_type from_account;
    account_id_type to_account;
    uint16_t percent;
    bool auto_vest;
};
struct escrow_object {
    id_type id;
    uint32_t escrow_id;
    account_name_type from;
    account_name_type to;
    account_name_type agent;
    time_point_sec ratification_deadline;
    time_point_sec escrow_expiration;
    asset sbd_balance;
    asset steem_balance;
    asset pending_fee;
    bool to_approved;
    bool agent_approved;
    bool disputed;
};
struct savings_withdraw_object {
    id_type id;
    account_name_type from;
    account_name_type to;
    shared_string memo;
    uint32_t request_id;
    asset amount;
    time_point_sec complete;
};
struct decline_voting_rights_request_object {
    id_type id;
    account_id_type account;
    time_point_sec effective_date;
};


// empty; unsupported, must not exist in serialized state
struct transaction_object {};
struct feed_history_object {};
struct hardfork_property_object {};
struct block_stats_object {};
struct proposal_object {};
struct required_approval_object {};

} // golos


enum object_type {
    dynamic_global_property_object_id,
    account_object_id,
    account_authority_object_id,
    account_bandwidth_object_id,
    witness_object_id,
    transaction_object_id,
    block_summary_object_id,
    witness_schedule_object_id,
    comment_object_id,
    comment_vote_object_id,
    witness_vote_object_id,
    limit_order_object_id,
    feed_history_object_id,
    convert_request_object_id,
    liquidity_reward_balance_object_id,
    hardfork_property_object_id,
    withdraw_vesting_route_object_id,
    owner_authority_history_object_id,
    account_recovery_request_object_id,
    change_recovery_account_request_object_id,
    escrow_object_id,
    savings_withdraw_object_id,
    decline_voting_rights_request_object_id,
    block_stats_object_id,
    vesting_delegation_object_id,
    vesting_delegation_expiration_object_id,
    account_metadata_object_id,
    proposal_object_id,
    required_approval_object_id
};

using namespace cw::golos;
// variant must be ordered the same way as enum (including not required objects):
using objects = fc::static_variant<
    dynamic_global_property_object,
    account_object,
    account_authority_object,
    account_bandwidth_object,
    witness_object,
    transaction_object,
    block_summary_object,
    witness_schedule_object,
    comment_object,
    comment_vote_object,
    witness_vote_object,
    limit_order_object,
    feed_history_object,
    convert_request_object,
    liquidity_reward_balance_object,
    hardfork_property_object,
    withdraw_vesting_route_object,
    owner_authority_history_object,
    account_recovery_request_object,
    change_recovery_account_request_object,
    escrow_object,
    savings_withdraw_object,
    decline_voting_rights_request_object,
    block_stats_object,
    vesting_delegation_object,
    vesting_delegation_expiration_object,
    account_metadata_object,
    proposal_object,
    required_approval_object
>;

} // cw
