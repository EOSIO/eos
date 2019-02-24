#include "golos_objects.hpp"
#include <fc/reflect/reflect.hpp>


FC_REFLECT(cw::gls_acc_name, (value))

// base
FC_REFLECT(cw::golos::shared_authority, (weight_threshold)(account_auths)(key_auths))
FC_REFLECT(cw::golos::chain_properties_17, (account_creation_fee)(maximum_block_size)(sbd_interest_rate))
FC_REFLECT_DERIVED(cw::golos::chain_properties_18, (cw::golos::chain_properties_17),
    (create_account_min_golos_fee)(create_account_min_delegation)(create_account_delegation_time)(min_delegation))
FC_REFLECT_DERIVED(cw::golos::chain_properties_19, (cw::golos::chain_properties_18),
    (max_referral_interest_rate)(max_referral_term_sec)(min_referral_break_fee)(max_referral_break_fee)
    (posts_window)(posts_per_window)(comments_window)(comments_per_window)(votes_window)(votes_per_window)
    (auction_window_size)(max_delegated_vesting_interest_rate)(custom_ops_bandwidth_multiplier)
    (min_curation_percent)(max_curation_percent)(curation_reward_curve)
    (allow_distribute_auction_reward)(allow_return_auction_reward_to_fund))
FC_REFLECT(cw::golos::price, (base)(quote))
FC_REFLECT(cw::golos::beneficiary_route_type, (account)(weight))


// objects
FC_REFLECT(cw::golos::dynamic_global_property_object,
    (id)(head_block_number)(head_block_id)(time)(current_witness)(total_pow)(num_pow_witnesses)
    (virtual_supply)(current_supply)(confidential_supply)(current_sbd_supply)(confidential_sbd_supply)
    (total_vesting_fund_steem)(total_vesting_shares)(total_reward_fund_steem)(total_reward_shares2)
    (sbd_interest_rate)(sbd_print_rate)(average_block_size)(maximum_block_size)
    (current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)(max_virtual_bandwidth)
    (current_reserve_ratio)(vote_regeneration_per_day)(custom_ops_bandwidth_multiplier)(is_forced_min_price)
)

FC_REFLECT(cw::golos::account_object,
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
FC_REFLECT(cw::golos::account_authority_object, (id)(account)(owner)(active)(posting)(last_owner_update))
FC_REFLECT(cw::golos::account_bandwidth_object,
    (id)(account)(type)(average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update))
FC_REFLECT(cw::golos::account_metadata_object, (id)(account)(json_metadata))
FC_REFLECT(cw::golos::vesting_delegation_object,
    (id)(delegator)(delegatee)(vesting_shares)(interest_rate)(min_delegation_time))
FC_REFLECT(cw::golos::vesting_delegation_expiration_object, (id)(delegator)(vesting_shares)(expiration))
FC_REFLECT(cw::golos::owner_authority_history_object, (id)(account)(previous_owner_authority)(last_valid_time))
FC_REFLECT(cw::golos::account_recovery_request_object, (id)(account_to_recover)(new_owner_authority)(expires))
FC_REFLECT(cw::golos::change_recovery_account_request_object,
    (id)(account_to_recover)(recovery_account)(effective_on))

FC_REFLECT(cw::golos::witness_object,
    (id)(owner)(created)(url)(votes)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
    (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)(props)(sbd_exchange_rate)(last_sbd_exchange_update)
    (last_work)(running_version)(hardfork_version_vote)(hardfork_time_vote))
FC_REFLECT(cw::golos::witness_schedule_object,
    (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_witnesses)(num_scheduled_witnesses)
    (top19_weight)(timeshare_weight)(miner_weight)(witness_pay_normalization_factor)
    (median_props)(majority_version))
FC_REFLECT(cw::golos::witness_vote_object, (id)(witness)(account))

FC_REFLECT(cw::golos::comment_object,
    (id)(parent_author)(parent_permlink)(author)(permlink)(created)(last_payout)(depth)(children)
    (children_rshares2)(net_rshares)(abs_rshares)(vote_rshares)(children_abs_rshares)(cashout_time)(max_cashout_time)
    (reward_weight)(net_votes)(total_votes)(root_comment)(mode)
    (curation_reward_curve)(auction_window_reward_destination)(auction_window_size)(max_accepted_payout)
    (percent_steem_dollars)(allow_replies)(allow_votes)(allow_curation_rewards)(curation_rewards_percent)
    (beneficiaries))
FC_REFLECT(cw::golos::delegator_vote_interest_rate, (account)(interest_rate)(payout_strategy))
FC_REFLECT(cw::golos::comment_vote_object,
    (id)(voter)(comment)(orig_rshares)(rshares)(vote_percent)(auction_time)(last_update)(num_changes)
    (delegator_vote_interest_rates))


FC_REFLECT(cw::golos::limit_order_object, (id)(created)(expiration)(seller)(orderid)(for_sale)(sell_price))
FC_REFLECT(cw::golos::convert_request_object, (id)(owner)(requestid)(amount)(conversion_date))
FC_REFLECT(cw::golos::liquidity_reward_balance_object, (id)(owner)(steem_volume)(sbd_volume)(weight)(last_update))
FC_REFLECT(cw::golos::withdraw_vesting_route_object, (id)(from_account)(to_account)(percent)(auto_vest))
FC_REFLECT(cw::golos::escrow_object,
    (id)(escrow_id)(from)(to)(agent)(ratification_deadline)(escrow_expiration)
    (sbd_balance)(steem_balance)(pending_fee)(to_approved)(agent_approved)(disputed))
FC_REFLECT(cw::golos::savings_withdraw_object, (id)(from)(to)(memo)(request_id)(amount)(complete))
FC_REFLECT(cw::golos::decline_voting_rights_request_object, (id)(account)(effective_date))
