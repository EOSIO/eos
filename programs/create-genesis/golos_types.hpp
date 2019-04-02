#pragma once
#include <eosio/chain/asset.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/public_key.hpp>

namespace cyberway { namespace golos {


struct gls_mapped_str {
    fc::unsigned_int id;

    std::string value(const std::vector<std::string>& lookup) const {
        return lookup.at(id.value);
    };
};

using fc::uint128_t;
using fc::time_point_sec;
using eosio::chain::asset;
using eosio::chain::share_type;

using account_name_type = gls_mapped_str;//gls_acc_name;
using shared_permlink   = gls_mapped_str;
using shared_string     = std::string;
using id_type       = int64_t;
using digest_type   = fc::sha256;
using block_id_type = fc::ripemd160;
using weight_type   = uint16_t;

using public_key_type = fc::ecc::public_key_data;   // eosio public key is static variant: ecc/r1

using witness_id_type = id_type;
using account_id_type = id_type;
using comment_id_type = id_type;

struct shared_authority {
    uint32_t weight_threshold;
    std::vector<std::pair<account_name_type, weight_type>> account_auths;   // was map, but it requires to have operator< for accs
    std::map<public_key_type, weight_type> key_auths;
};


struct chain_properties_17 {
    asset account_creation_fee;
    uint32_t maximum_block_size;
    uint16_t sbd_interest_rate;
};
struct chain_properties_18: public chain_properties_17 {
    asset create_account_min_golos_fee;
    asset create_account_min_delegation;
    uint32_t create_account_delegation_time;
    asset min_delegation;
};
enum class curation_curve: uint8_t {bounded, linear, square_root, _size, detect = 100};
struct chain_properties_19: public chain_properties_18 {
    uint16_t max_referral_interest_rate;
    uint32_t max_referral_term_sec;
    asset min_referral_break_fee;
    asset max_referral_break_fee;
    uint16_t posts_window;
    uint16_t posts_per_window;
    uint16_t comments_window;
    uint16_t comments_per_window;
    uint16_t votes_window;
    uint16_t votes_per_window;
    uint16_t auction_window_size;
    uint16_t max_delegated_vesting_interest_rate;
    uint16_t custom_ops_bandwidth_multiplier;
    uint16_t min_curation_percent;
    uint16_t max_curation_percent;
    curation_curve curation_reward_curve;
    bool allow_return_auction_reward_to_fund;
    bool allow_distribute_auction_reward;
};
using chain_properties = chain_properties_19;

struct price {
    asset base;
    asset quote;
};

using version = uint32_t;
using hardfork_version = version;

struct beneficiary_route_type {
    account_name_type account;
    uint16_t weight;
};

enum bandwidth_type {post, forum, market, custom_json};
enum delegator_payout_strategy {to_delegator, to_delegated_vesting};
enum witness_schedule_type {top19, timeshare, miner, none};
enum comment_mode {not_set, first_payout, second_payout, archived};
enum auction_window_reward_destination_type {to_reward_fund, to_curators, to_author};

struct delegator_vote_interest_rate {
    account_name_type account;
    uint16_t interest_rate;
    delegator_payout_strategy payout_strategy;
};


}} // cyberway::golos


FC_REFLECT(cyberway::golos::gls_mapped_str, (id))

FC_REFLECT(cyberway::golos::shared_authority, (weight_threshold)(account_auths)(key_auths))
FC_REFLECT(cyberway::golos::chain_properties_17, (account_creation_fee)(maximum_block_size)(sbd_interest_rate))
FC_REFLECT_DERIVED(cyberway::golos::chain_properties_18, (cyberway::golos::chain_properties_17),
    (create_account_min_golos_fee)(create_account_min_delegation)(create_account_delegation_time)(min_delegation))
FC_REFLECT_DERIVED(cyberway::golos::chain_properties_19, (cyberway::golos::chain_properties_18),
    (max_referral_interest_rate)(max_referral_term_sec)(min_referral_break_fee)(max_referral_break_fee)
    (posts_window)(posts_per_window)(comments_window)(comments_per_window)(votes_window)(votes_per_window)
    (auction_window_size)(max_delegated_vesting_interest_rate)(custom_ops_bandwidth_multiplier)
    (min_curation_percent)(max_curation_percent)(curation_reward_curve)
    (allow_distribute_auction_reward)(allow_return_auction_reward_to_fund))
FC_REFLECT(cyberway::golos::price, (base)(quote))
FC_REFLECT(cyberway::golos::beneficiary_route_type, (account)(weight))
FC_REFLECT(cyberway::golos::delegator_vote_interest_rate, (account)(interest_rate)(payout_strategy))

FC_REFLECT_ENUM(cyberway::golos::comment_mode,    (not_set)(first_payout)(second_payout)(archived))
FC_REFLECT_ENUM(cyberway::golos::bandwidth_type,  (post)(forum)(market)(custom_json))
FC_REFLECT_ENUM(cyberway::golos::curation_curve,  (detect)(bounded)(linear)(square_root)(_size))
FC_REFLECT_ENUM(cyberway::golos::witness_schedule_type, (top19)(timeshare)(miner)(none))
FC_REFLECT_ENUM(cyberway::golos::delegator_payout_strategy, (to_delegator)(to_delegated_vesting))
FC_REFLECT_ENUM(cyberway::golos::auction_window_reward_destination_type, (to_reward_fund)(to_curators)(to_author))
