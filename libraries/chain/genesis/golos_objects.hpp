#pragma once
#include "golos_types.hpp"

namespace cyberway { namespace golos {


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
    // fc::array<account_name_type, 21/*STEEMIT_MAX_WITNESSES*/> current_shuffled_witnesses;
    fc::array<uint128_t, 21/*STEEMIT_MAX_WITNESSES*/> current_shuffled_witnesses;
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
// this part of comment object is absent in archived comments
struct active_comment_data {
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
struct comment_object {
    id_type id;
    account_name_type parent_author;
    shared_permlink parent_permlink;
    account_name_type author;
    shared_permlink permlink;
    comment_mode mode;
    active_comment_data active;
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


// other
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


// empty; unsupported, must not exist in serialized state; TODO: remove
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

// variant must be ordered the same way as enum (including not required objects):
using objects = fc::static_variant<
    golos::dynamic_global_property_object,
    golos::account_object,
    golos::account_authority_object,
    golos::account_bandwidth_object,
    golos::witness_object,
    golos::transaction_object,
    golos::block_summary_object,
    golos::witness_schedule_object,
    golos::comment_object,
    golos::comment_vote_object,
    golos::witness_vote_object,
    golos::limit_order_object,
    golos::feed_history_object,
    golos::convert_request_object,
    golos::liquidity_reward_balance_object,
    golos::hardfork_property_object,
    golos::withdraw_vesting_route_object,
    golos::owner_authority_history_object,
    golos::account_recovery_request_object,
    golos::change_recovery_account_request_object,
    golos::escrow_object,
    golos::savings_withdraw_object,
    golos::decline_voting_rights_request_object,
    golos::block_stats_object,
    golos::vesting_delegation_object,
    golos::vesting_delegation_expiration_object,
    golos::account_metadata_object,
    golos::proposal_object,
    golos::required_approval_object
>;

} // cyberway


FC_REFLECT(cyberway::golos::dynamic_global_property_object,
    (id)(head_block_number)(head_block_id)(time)(current_witness)(total_pow)(num_pow_witnesses)
    (virtual_supply)(current_supply)(confidential_supply)(current_sbd_supply)(confidential_sbd_supply)
    (total_vesting_fund_steem)(total_vesting_shares)(total_reward_fund_steem)(total_reward_shares2)
    (sbd_interest_rate)(sbd_print_rate)(average_block_size)(maximum_block_size)
    (current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)(max_virtual_bandwidth)
    (current_reserve_ratio)(vote_regeneration_per_day)(custom_ops_bandwidth_multiplier)(is_forced_min_price)
)

FC_REFLECT(cyberway::golos::account_object,
    (id)(name)(memo_key)(proxy)(last_account_update)(created)(mined)
    (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)
    (recovery_account)(last_account_recovery)(reset_account)
    (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_power)(last_vote_time)
    (balance)(savings_balance)(sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
    (savings_sbd_balance)(savings_sbd_seconds)(savings_sbd_seconds_last_update)
    (savings_sbd_last_interest_payment)(savings_withdraw_requests)
    (vesting_shares)(delegated_vesting_shares)(received_vesting_shares)
    (vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
    (benefaction_rewards)(curation_rewards)(delegation_rewards)(posting_rewards)
    (proxied_vsf_votes)(witnesses_voted_for)
    (last_post)
    (referrer_account)(referrer_interest_rate)(referral_end_date)(referral_break_fee)
)
FC_REFLECT(cyberway::golos::account_authority_object, (id)(account)(owner)(active)(posting)(last_owner_update))
FC_REFLECT(cyberway::golos::account_bandwidth_object,
    (id)(account)(type)(average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update))
FC_REFLECT(cyberway::golos::account_metadata_object, (id)(account)(json_metadata))
FC_REFLECT(cyberway::golos::vesting_delegation_object,
    (id)(delegator)(delegatee)(vesting_shares)(interest_rate)(min_delegation_time))
FC_REFLECT(cyberway::golos::vesting_delegation_expiration_object, (id)(delegator)(vesting_shares)(expiration))
FC_REFLECT(cyberway::golos::owner_authority_history_object, (id)(account)(previous_owner_authority)(last_valid_time))
FC_REFLECT(cyberway::golos::account_recovery_request_object, (id)(account_to_recover)(new_owner_authority)(expires))
FC_REFLECT(cyberway::golos::change_recovery_account_request_object,
    (id)(account_to_recover)(recovery_account)(effective_on))

FC_REFLECT(cyberway::golos::witness_object,
    (id)(owner)(created)(url)(votes)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
    (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)(props)(sbd_exchange_rate)(last_sbd_exchange_update)
    (last_work)(running_version)(hardfork_version_vote)(hardfork_time_vote))
FC_REFLECT(cyberway::golos::witness_schedule_object,
    (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_witnesses)(num_scheduled_witnesses)
    (top19_weight)(timeshare_weight)(miner_weight)(witness_pay_normalization_factor)
    (median_props)(majority_version))
FC_REFLECT(cyberway::golos::witness_vote_object, (id)(witness)(account))

FC_REFLECT(cyberway::golos::block_summary_object, (id)(block_id))

// comment must be unpacked manually
// FC_REFLECT(cyberway::golos::comment_object, (id)(parent_author)(parent_permlink)(author)(permlink)(mode))
FC_REFLECT(cyberway::golos::active_comment_data,
    (created)(last_payout)(depth)(children)
    (children_rshares2)(net_rshares)(abs_rshares)(vote_rshares)(children_abs_rshares)(cashout_time)(max_cashout_time)
    (reward_weight)(net_votes)(total_votes)(root_comment)
    (curation_reward_curve)(auction_window_reward_destination)(auction_window_size)(max_accepted_payout)
    (percent_steem_dollars)(allow_replies)(allow_votes)(allow_curation_rewards)(curation_rewards_percent)
    (beneficiaries))
FC_REFLECT(cyberway::golos::comment_vote_object,
    (id)(voter)(comment)(orig_rshares)(rshares)(vote_percent)(auction_time)(last_update)(num_changes)
    (delegator_vote_interest_rates))


FC_REFLECT(cyberway::golos::limit_order_object, (id)(created)(expiration)(seller)(orderid)(for_sale)(sell_price))
FC_REFLECT(cyberway::golos::convert_request_object, (id)(owner)(requestid)(amount)(conversion_date))
FC_REFLECT(cyberway::golos::liquidity_reward_balance_object, (id)(owner)(steem_volume)(sbd_volume)(weight)(last_update))
FC_REFLECT(cyberway::golos::withdraw_vesting_route_object, (id)(from_account)(to_account)(percent)(auto_vest))
FC_REFLECT(cyberway::golos::escrow_object,
    (id)(escrow_id)(from)(to)(agent)(ratification_deadline)(escrow_expiration)
    (sbd_balance)(steem_balance)(pending_fee)(to_approved)(agent_approved)(disputed))
FC_REFLECT(cyberway::golos::savings_withdraw_object, (id)(from)(to)(memo)(request_id)(amount)(complete))
FC_REFLECT(cyberway::golos::decline_voting_rights_request_object, (id)(account)(effective_date))

FC_REFLECT(cyberway::golos::transaction_object,       BOOST_PP_SEQ_NIL)
FC_REFLECT(cyberway::golos::feed_history_object,      BOOST_PP_SEQ_NIL)
FC_REFLECT(cyberway::golos::hardfork_property_object, BOOST_PP_SEQ_NIL)
FC_REFLECT(cyberway::golos::block_stats_object,       BOOST_PP_SEQ_NIL)
FC_REFLECT(cyberway::golos::proposal_object,          BOOST_PP_SEQ_NIL)
FC_REFLECT(cyberway::golos::required_approval_object, BOOST_PP_SEQ_NIL)
