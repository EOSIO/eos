#pragma once
#include "custom_unpack.hpp"
#include <eosio/chain/asset.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/public_key.hpp>
#include <string>


namespace cw {
using std::string;
using eosio::chain::share_type;
}

namespace cw { namespace golos {


using account_name_type = gls_acc_name;
template<int N>
using shared_string = gls_shared_str<N>;
using asset         = gls_asset;
using id_type       = int64_t;
using digest_type   = fc::sha256;
using block_id_type = fc::ripemd160;
using weight_type   = uint16_t;

using public_key_type = fc::ecc::public_key_data;   // eosio public key is static variant: ecc/r1

struct shared_authority {
    uint32_t weight_threshold;
    std::map<account_name_type, weight_type> account_auths;
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

enum sstr_type {permlink, url, meta, memo};

}} // cw::golos
